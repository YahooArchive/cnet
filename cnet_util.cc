// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Some portions of this file:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "url/gurl.h"
#include "yahoo/cnet/cnet.h"

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#endif

#if !defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
#include "base/i18n/icu_util.h"
#endif

namespace {

// LICENSE: copied from base/strings/string_split.cc
//  (it isn't exposed via a header file)
bool SplitStringIntoKeyValue(const std::string& line,
                             char key_value_delimiter,
                             std::string* key,
                             std::string* value) {
  key->clear();
  value->clear();

  // Find the delimiter.
  size_t end_key_pos = line.find_first_of(key_value_delimiter);
  if (end_key_pos == std::string::npos) {
    DVLOG(1) << "cannot find delimiter in: " << line;
    return false;    // no delimiter
  }
  key->assign(line, 0, end_key_pos);

  // Find the value string.
  std::string remains(line, end_key_pos, line.size() - end_key_pos);
  size_t begin_value_pos = remains.find_first_not_of(key_value_delimiter);
  if (begin_value_pos == std::string::npos) {
    DVLOG(1) << "cannot parse value from line: " << line;
    return false;   // no value
  }
  value->assign(remains, begin_value_pos, remains.size() - begin_value_pos);
  return true;
}

void GetCompletion(CnetFetcher fetcher, CnetResponse response, void* param) {
  // On network thread.
  const char* initial_url = CnetResponseInitialUrl(response);
  int http_response_code = CnetResponseHttpCode(response);
  const char* response_body = CnetResponseBody(response);
  int response_len = CnetResponseLength(response);
  const CnetLoadTiming* timing = CnetResponseTiming(response);
  char* content_type = CnetResponseFirstHeaderCopy(response, "Content-Type");
  if (response_body != NULL) {
    write(STDOUT_FILENO, response_body, response_len);
  }
  if (initial_url != NULL) {
    LOG(INFO) << "Initial URL: " << initial_url;
  }
  LOG(INFO) << "HTTP result: " << http_response_code;
  if (content_type != NULL) {
    LOG(INFO) << "Content-Type: " << content_type;
  }
  LOG(INFO) << "size: " << response_len;
  LOG(INFO) << "dns (ms): " << timing->dns_ms;
  LOG(INFO) << "connect (ms): " << timing->connect_ms;
  LOG(INFO) << "ssl (ms): " << timing->ssl_ms;
  LOG(INFO) << "proxy (ms): " << timing->proxy_resolve_ms;
  LOG(INFO) << "send (ms): " << timing->send_ms;
  LOG(INFO) << "headers receive (ms): " << timing->headers_receive_ms;
  LOG(INFO) << "data receive (ms): " << timing->data_receive_ms;
  if (CnetResponseWasCached(response)) {
    LOG(INFO) << "from cache";
  }
  if (CnetResponseWasFetchedViaProxy(response)) {
    LOG(INFO) << "fetched via a proxy";
  }
  if (CnetResponseWasFetchedViaHttp(response)) {
    LOG(INFO) << "fetched via HTTP";
  }
  if (CnetResponseWasFetchedViaSpdy(response)) {
    LOG(INFO) << "fetched via SPDY";
  }
  if (CnetResponseWasFetchedViaQuic(response)) {
    LOG(INFO) << "fetched via QUIC";
  }

  if (content_type != NULL) {
    free(content_type);
  }
  CnetPool pool = CnetFetcherPool(fetcher);
  CnetFetcherRelease(fetcher);
  CnetPoolRelease(pool);

  // Quit the main loop, but give some time for the pool threads to
  // join with the UI thread.
  base::MessageLoopForUI* ui_loop = (base::MessageLoopForUI*)param;
  ui_loop->PostDelayedTask(FROM_HERE, base::MessageLoopForUI::QuitClosure(),
      base::TimeDelta::FromMilliseconds(300));
}

void UploadProgress(CnetFetcher fetcher, void* param,
    int64_t current, int64_t expected) {
  // On network thread.
  LOG(INFO) << "sent " << current << " of " << expected;
}

void GetProgress(CnetFetcher fetcher, void* param,
    int64_t current, int64_t expected) {
  // On network thread.
  LOG(INFO) << "got " << current << " of " << expected;
}

} // namespace


int main(int argc, char* argv[]) {
#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool auto_release_pool;
#endif
  base::AtExitManager exit_manager;
  base::StatisticsRecorder::Initialize();
#if !defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
  base::i18n::InitializeICU();
#endif

  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  std::string cache_path(command_line.GetSwitchValueASCII("cache"));
  std::string proxy_rules(command_line.GetSwitchValueASCII("proxy-rules"));
  std::string cache_behavior(command_line.GetSwitchValueASCII("cache-behavior"));
  bool trust_all_cert_authorities(command_line.HasSwitch(
      "trust-all-cert-authorities"));
  std::string oauth_app_key(command_line.GetSwitchValueASCII("oauth-app-key"));
  std::string oauth_app_secret(command_line.GetSwitchValueASCII(
      "oauth-app-secret"));
  std::string oauth_token(command_line.GetSwitchValueASCII("oauth-token"));
  std::string oauth_token_secret(command_line.GetSwitchValueASCII(
      "oauth-token-secret"));
  std::string method(command_line.GetSwitchValueASCII("method"));
  if (method.empty()) {
    method = "GET";
  }
  std::string body(command_line.GetSwitchValueASCII("body"));
  std::string body_type(command_line.GetSwitchValueASCII("body-type"));
  base::FilePath upload_path(command_line.GetSwitchValueASCII("upload"));
  std::string upload_content_type(command_line.GetSwitchValueASCII(
      "upload-type"));
  std::string upload_key(command_line.GetSwitchValueASCII("upload-key"));
  std::string output_path(command_line.GetSwitchValueASCII("output-path"));
  std::string min_speed(command_line.GetSwitchValueASCII("min-speed"));
  std::string quic_host(command_line.GetSwitchValueASCII("quic-host"));
  std::string quic_port_str(command_line.GetSwitchValueASCII("quic-port"));

  base::MessageLoopForUI ui_loop;

  CnetInitialize(1);

  CnetPoolConfig pool_config;
  CnetPoolDefaultConfigPrepare(&pool_config);
  pool_config.user_agent = "cnet-util";
  pool_config.enable_spdy = 1;
  pool_config.enable_quic = 1;
  pool_config.cache_path = (cache_path.empty()) ? NULL:cache_path.c_str();
  pool_config.cache_max_bytes = 40*1024;
  pool_config.trust_all_cert_authorities = trust_all_cert_authorities;
  pool_config.log_level = 1;
  CnetPool pool = CnetPoolCreate(static_cast<CnetMessageLoopForUi>(&ui_loop),
      pool_config);
  CnetPoolSetProxy(pool, proxy_rules.c_str());
  if (!quic_host.empty() && !quic_port_str.empty()) {
    unsigned quic_port;
    if (base::StringToUint(quic_port_str, &quic_port)) {
      if (quic_port > std::numeric_limits<uint16>::min() &&
          quic_port <= std::numeric_limits<uint16>::max()) {
        CnetPoolAddQuicHint(pool, quic_host.c_str(), 80, quic_port);
        CnetPoolAddQuicHint(pool, quic_host.c_str(), 443, quic_port);
      }
    }
  }

  std::string params_encoding(command_line.GetSwitchValueASCII("encoding"));
  base::StringPairs fetch_params;
  base::StringPairs fetch_headers;
  GURL url;
  base::CommandLine::StringVector args = command_line.GetArgs();
  for (base::CommandLine::StringVector::const_iterator arg_it = args.begin();
       arg_it != args.end(); arg_it++) {
    GURL url_try = GURL(*arg_it);
    if (url_try.is_valid() && url_try.IsStandard()) {
      url = url_try;
    } else {
      std::string key;
      std::string value;
      if (SplitStringIntoKeyValue(*arg_it, '=', &key, &value)) {
        std::pair<std::string, std::string> pair(key, value);
        fetch_params.push_back(pair);
      } else if (SplitStringIntoKeyValue(*arg_it, ':', &key, &value)) {
        std::pair<std::string, std::string> pair(key, value);
        fetch_headers.push_back(pair);
      }
    }
  }

  CnetFetcher fetcher = CnetFetcherCreate(pool, url.spec().c_str(),
      method.c_str(), &ui_loop, GetCompletion, GetProgress, UploadProgress);
  if (fetcher != NULL) {
    if (!min_speed.empty()) {
      double bytes_sec = 0;
      if (base::StringToDouble(min_speed, &bytes_sec)) {
        CnetFetcherSetMinSpeed(fetcher, bytes_sec, 1.0);
      }
    }
    if (!output_path.empty()) {
      CnetFetcherSetOutputFile(fetcher, output_path.c_str());
    }
    if (!oauth_app_key.empty()) {
      CnetFetcherSetOauthCredentials(fetcher,
          oauth_app_key.c_str(), oauth_app_secret.c_str(),
          oauth_token.c_str(), oauth_token_secret.c_str());
    }
    if (params_encoding == "url") {
      CnetFetcherSetUrlParamsEncoding(fetcher, CNET_ENCODE_URL);
    } else if (params_encoding == "body-url") {
      CnetFetcherSetUrlParamsEncoding(fetcher, CNET_ENCODE_BODY_URL);
    } else if (params_encoding == "body-multipart") {
      CnetFetcherSetUrlParamsEncoding(fetcher, CNET_ENCODE_BODY_MULTIPART);
    }
    if (cache_behavior == "validate") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_VALIDATE);
    } else if (cache_behavior == "bypass") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_BYPASS);
    } else if (cache_behavior == "prefer") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_PREFER);
    } else if (cache_behavior == "only") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_ONLY);
    } else if (cache_behavior == "offline") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_IF_OFFLINE);
    } else if (cache_behavior == "disable") {
      CnetFetcherSetCacheBehavior(fetcher, CNET_CACHE_DISABLE);
    }
    for (base::StringPairs::const_iterator it = fetch_headers.begin();
         it != fetch_headers.end(); ++it) {
      CnetFetcherSetHeader(fetcher, it->first.c_str(), it->second.c_str());
    }
    for (base::StringPairs::const_iterator it = fetch_params.begin();
         it != fetch_params.end(); ++it) {
      CnetFetcherSetUrlParam(fetcher, it->first.c_str(), it->second.c_str());
    }
    if (!body.empty()) {
      CnetFetcherSetUploadBody(fetcher, body_type.c_str(), body.c_str());
    } else if (!upload_path.empty()) {
      if (params_encoding == "body-multipart") {
        CnetFetcherSetUrlParamFile(fetcher, upload_key.c_str(),
            upload_path.BaseName().AsUTF8Unsafe().c_str(),
            upload_content_type.c_str(), upload_path.AsUTF8Unsafe().c_str(),
            0, kuint64max);
      } else {
        CnetFetcherSetUploadFile(fetcher, upload_content_type.c_str(),
            upload_path.AsUTF8Unsafe().c_str(), 0, kuint64max);
      }
    }
    CnetFetcherStart(fetcher);

    ui_loop.Run();
  } else {
    CnetPoolRelease(pool);
  }

  CnetCleanup();

  return EXIT_SUCCESS;
}
