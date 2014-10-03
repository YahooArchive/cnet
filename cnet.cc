// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/cnet.h"

#include <string>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/cnet_pool.h"
#include "yahoo/cnet/cnet_fetcher.h"
#include "yahoo/cnet/cnet_oauth.h"
#include "yahoo/cnet/cnet_response.h"

#if !defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
#include "base/i18n/icu_util.h"
#endif

namespace cnet {

base::AtExitManager* g_at_exit_manager = NULL;
CnetMessageLoopForUi g_ui_loop = NULL;

void Initialize(bool in_chromium) {
  if (!in_chromium && (g_at_exit_manager == NULL)) {
    g_at_exit_manager = new base::AtExitManager();

    base::StatisticsRecorder::Initialize();
#if !defined(USE_ICU_ALTERNATIVES_ON_ANDROID)
    base::i18n::InitializeICU();
#endif

    base::CommandLine::Init(0, NULL);

    logging::LoggingSettings settings;
    settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;
    logging::InitLogging(settings);
  }
}

void Cleanup() {
  if (g_at_exit_manager != NULL) {
    delete g_at_exit_manager;
    g_at_exit_manager = NULL;
  }
}

class PoolSyncDrainer : public Pool::Observer {
 public:
  PoolSyncDrainer(scoped_refptr<Pool> pool);
  virtual ~PoolSyncDrainer();

  void Drain();

  virtual void OnPoolIdle(scoped_refptr<Pool> pool) OVERRIDE;

 private:
  void OnWorkDrained();

  base::WaitableEvent event_;
  scoped_refptr<Pool> pool_;
};


PoolSyncDrainer::PoolSyncDrainer(scoped_refptr<Pool> pool)
  : event_(false, false), pool_(pool) {
  pool_->AddObserver(this);
}

PoolSyncDrainer::~PoolSyncDrainer() {
  pool_->RemoveObserver(this);
}

void PoolSyncDrainer::OnPoolIdle(scoped_refptr<Pool> pool) {
  // On network thread.
  pool_->GetWorkTaskRunner()->PostTask(FROM_HERE,
      base::Bind(&PoolSyncDrainer::OnWorkDrained, base::Unretained(this)));
}

void PoolSyncDrainer::OnWorkDrained() {
  // On work thread.
  event_.Signal();
}

void PoolSyncDrainer::Drain() {
  event_.Wait();
}

} // namespace cnet


extern "C" {

CnetMessageLoopForUi CnetMessageLoopForUiGet() {
  if (cnet::g_ui_loop == NULL) {
#if defined(OS_IOS)
    base::MessageLoopForUI* ui_loop = new base::MessageLoopForUI();
    cnet::g_ui_loop = static_cast<CnetMessageLoopForUi>(ui_loop);
    ui_loop->Attach();
#elif defined(OS_ANDROID)
    base::MessageLoopForUI* ui_loop = new base::MessageLoopForUI();
    cnet::g_ui_loop = static_cast<CnetMessageLoopForUi>(ui_loop);
    ui_loop->Start();
#endif
  }
  return cnet::g_ui_loop;
}

void CnetInitialize(int in_chromium) {
  cnet::Initialize(in_chromium != 0);
}

void CnetCleanup() {
  cnet::Cleanup();
}

void CnetPoolDefaultConfigPrepare(CnetPoolConfig* config) {
  memset(config, 0, sizeof(CnetPoolConfig));
}

CnetPool CnetPoolCreate(CnetMessageLoopForUi ui_loop,
    CnetPoolConfig pool_config) {
  scoped_refptr<base::SingleThreadTaskRunner> ui_runner;
  if (ui_loop != NULL) {
    ui_runner =
        reinterpret_cast<base::MessageLoopForUI*>(ui_loop)->task_runner();
  }

  cnet::Pool::Config config;
  config.user_agent = pool_config.user_agent != NULL ?
      pool_config.user_agent:"";
  config.enable_spdy = pool_config.enable_spdy != 0;
  config.enable_quic = pool_config.enable_quic != 0;
  config.enable_ssl_false_start = pool_config.enable_ssl_false_start != 0;
  config.disable_system_proxy = pool_config.disable_system_proxy != 0;
  config.cache_path = base::FilePath((pool_config.cache_path != NULL) ?
      pool_config.cache_path:"");
  config.cache_max_bytes = pool_config.cache_max_bytes;
  config.trust_all_cert_authorities = pool_config.trust_all_cert_authorities != 0;
  config.log_level = pool_config.log_level;

  cnet::Pool* pool = new cnet::Pool(ui_runner, config);
  if (pool != NULL) {
    pool->AddRef();
  }
  return pool;
}
    
CnetPool CnetPoolRetain(CnetPool pool) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->AddRef();
  }
  return pool;
}
  
void CnetPoolRelease(CnetPool pool) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->Release();
  }
}

void CnetPoolSetProxy(CnetPool pool, const char* rules) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->SetProxyConfig(rules ? rules:"");
  }
}

void CnetPoolSetEnableSslFalseStart(CnetPool pool, int enabled) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->SetEnableSslFalseStart(enabled != 0);
  }
}

void CnetPoolSetTrustAllCertAuthorities(CnetPool pool, int enabled) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->SetTrustAllCertAuthorities(enabled != 0);
  }
}

void CnetPoolDrain(CnetPool pool) {
  if (pool != NULL) {
    scoped_ptr<cnet::PoolSyncDrainer> drainer(
        new cnet::PoolSyncDrainer(static_cast<cnet::Pool*>(pool)));
    drainer->Drain();
  }
}

void CnetPoolTagFetcher(CnetPool pool, CnetFetcher fetcher, int tag) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->TagFetcher(
        static_cast<cnet::Fetcher*>(fetcher), tag);
  }
}

void CnetPoolCancelTag(CnetPool pool, int tag) {
  if (pool != NULL) {
    static_cast<cnet::Pool*>(pool)->CancelTag(tag);
  }
}

void CnetInvokeCompletion(CnetFetcherCompletion completion,
    void* callback_param, scoped_refptr<cnet::Fetcher> fetcher,
    scoped_refptr<cnet::Response> response) {
  if (completion != NULL) {
    completion(fetcher.get(), response.get(), callback_param);
  }
}

void CnetInvokeProgressCallback(CnetFetcherProgressCallback callback,
    void* callback_param, scoped_refptr<cnet::Fetcher> fetcher,
    int64_t current, int64_t total) {
  if (callback != NULL) {
    callback(fetcher.get(), callback_param, current, total);
  }
}

CnetFetcher CnetFetcherCreate(CnetPool pool, const char* url,
    const char* method, void* callback_param,
    CnetFetcherCompletion completion, CnetFetcherProgressCallback download,
    CnetFetcherProgressCallback upload) {
  if ((url == NULL) || (method == NULL)) {
    return NULL;
  }

  cnet::Fetcher::CompletionCallback completion_callback;
  if (completion != NULL) {
    completion_callback = base::Bind(CnetInvokeCompletion,
        completion, callback_param);
  }
  cnet::Fetcher::ProgressCallback download_callback;
  if (download != NULL) {
    download_callback = base::Bind(CnetInvokeProgressCallback,
        download, callback_param);
  }
  cnet::Fetcher::ProgressCallback upload_callback;
  if (upload != NULL) {
    upload_callback = base::Bind(CnetInvokeProgressCallback,
        upload, callback_param);
  }

  cnet::Fetcher* fetcher = new cnet::Fetcher(
      static_cast<cnet::Pool*>(pool), url, method,
      completion_callback, download_callback, upload_callback);
  if (fetcher != NULL) {
    fetcher->set_user_data(callback_param);

    fetcher->AddRef();
  }
  return fetcher;
}

CnetFetcher CnetFetcherRetain(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->AddRef();
  }
  return fetcher;
}

void CnetFetcherRelease(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->Release();
  }
}

void CnetFetcherStart(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->Start();
  }
}

void CnetFetcherCancel(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->Cancel();
  }
}

void* CnetFetcherGetCallbackParam(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    return static_cast<cnet::Fetcher*>(fetcher)->get_user_data();
  } else {
    return NULL;
  }
}

void CnetFetcherSetMinSpeed(CnetFetcher fetcher,
  double min_speed_bytes_sec, double duration_secs) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->SetMinSpeed(
        min_speed_bytes_sec, duration_secs);
  }
}

void CnetFetcherSetCacheBehavior(CnetFetcher raw_fetcher,
    CnetCacheBehavior behavior) {
  if (raw_fetcher != NULL) {
    cnet::Fetcher* fetcher = static_cast<cnet::Fetcher*>(raw_fetcher);
    switch (behavior) {
      case CNET_CACHE_BYPASS:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_BYPASS);
        break;
      case CNET_CACHE_DISABLE:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_DISABLE);
        break;
      case CNET_CACHE_IF_OFFLINE:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_IF_OFFLINE);
        break;
      case CNET_CACHE_NORMAL:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_NORMAL);
        break;
      case CNET_CACHE_ONLY:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_ONLY);
        break;
      case CNET_CACHE_PREFER:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_PREFER);
        break;
      case CNET_CACHE_VALIDATE:
        fetcher->SetCacheBehavior(cnet::Fetcher::CACHE_VALIDATE);
        break;
    }
  }
}
  
void CnetFetcherSetStopOnRedirect(CnetFetcher fetcher,
    int stop_on_redirect) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->SetStopOnRedirect(
        stop_on_redirect != 0);
  }
}

void CnetFetcherSetHeader(CnetFetcher fetcher,
    const char* key, const char* value) {
  if ((fetcher != NULL) && (key != NULL) && (value != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetHeader(key, value);
  }
}

void CnetFetcherSetHeaderInt(CnetFetcher fetcher,
    const char* key, int value) {
  if ((fetcher != NULL) && (key != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetHeader(key,
        base::IntToString(value));
  }
}

void CnetFetcherSetHeaderDouble(CnetFetcher fetcher,
    const char* key, double value) {
  if ((fetcher != NULL) && (key != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetHeader(key,
        base::DoubleToString(value));
  }
}

void CnetFetcherSetUrlParamsEncoding(CnetFetcher fetcher,
    CnetUrlParamsEncoding encoding) {
  if (fetcher != NULL) {
    switch (encoding) {
    case CNET_ENCODE_BODY_MULTIPART:
      static_cast<cnet::Fetcher*>(fetcher)->SetUrlParamsEncoding(
          cnet::Fetcher::ENCODE_BODY_MULTIPART);
      break;
    case CNET_ENCODE_BODY_URL:
      static_cast<cnet::Fetcher*>(fetcher)->SetUrlParamsEncoding(
          cnet::Fetcher::ENCODE_BODY_URL);
      break;
    case CNET_ENCODE_URL:
      static_cast<cnet::Fetcher*>(fetcher)->SetUrlParamsEncoding(
          cnet::Fetcher::ENCODE_URL);
      break;
    }
  }
}

void CnetFetcherSetUrlParam(CnetFetcher fetcher,
    const char* key, const char* value) {
  if ((fetcher != NULL) && (key != NULL) && (value != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUrlParam(key, value);
  }
}

void CnetFetcherSetUrlParamInt(CnetFetcher fetcher,
    const char* key, int value) {
  if ((fetcher != NULL) && (key != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUrlParam(key,
        base::IntToString(value));
  }
}

void CnetFetcherSetUrlParamDouble(CnetFetcher fetcher,
    const char* key, double value) {
  if ((fetcher != NULL) && (key != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUrlParam(key,
        base::DoubleToString(value));
  }
}

void CnetFetcherSetUrlParamFile(CnetFetcher fetcher,
    const char* key, const char* filename, const char* content_type,
    const char* path, uint64_t range_offset, uint64_t range_length) {
  if ((fetcher != NULL) && (key != NULL)) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUrlParamFile(
        key,
        filename != NULL ? filename:"",
        content_type != NULL ? content_type:"",
        base::FilePath(path != NULL ? path:""),
        range_offset, range_length);
  }
}
  
void CnetFetcherSetOauthCredentials(CnetFetcher fetcher,
    const char* app_key, const char* app_secret, const char* token,
    const char* token_secret) {
  if (fetcher != NULL) {
    cnet::OauthCredentials credentials;
    credentials.app_key = app_key != NULL ? app_key:"";
    credentials.app_secret = app_secret != NULL ? app_secret:"";
    credentials.token = token != NULL ? token:"";
    credentials.token_secret = token_secret != NULL ? token_secret:"";
    static_cast<cnet::Fetcher*>(fetcher)->SetOauthCredentials(credentials);
  }
}

void CnetFetcherSetUploadBody(CnetFetcher fetcher,
    const char* content_type, const char* content) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUploadBody(
        content_type != NULL ? content_type:"",
        content != NULL ? content:"");
  }
}

void CnetFetcherSetUploadFile(CnetFetcher fetcher, const char* content_type,
    const char* path, uint64_t range_offset, uint64_t range_length) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->SetUploadFilePath(
        content_type != NULL ? content_type:"",
        base::FilePath(path != NULL ? path:""),
        range_offset, range_length);
  }
}

void CnetFetcherSetOutputFile(CnetFetcher fetcher, const char* path) {
  if (fetcher != NULL) {
    static_cast<cnet::Fetcher*>(fetcher)->SetOutputFilePath(
        base::FilePath(path != NULL ? path:""));
  }
}

CnetPool CnetFetcherPool(CnetFetcher fetcher) {
  if (fetcher != NULL) {
    return static_cast<cnet::Fetcher*>(fetcher)->pool().get();
  } else {
    return NULL;
  }
}

const char* CnetFetcherInitialUrl(CnetFetcher raw_fetcher) {
  if (raw_fetcher != NULL) {
    cnet::Fetcher* fetcher = static_cast<cnet::Fetcher*>(raw_fetcher);
    if (!fetcher->initial_url().empty()) {
      return fetcher->initial_url().c_str();
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

CnetResponse CnetResponseRetain(CnetResponse response) {
  if (response != NULL) {
    static_cast<cnet::Response*>(response)->AddRef();
  }
  return response;
}

void CnetResponseRelease(CnetResponse response) {
  if (response != NULL) {
    static_cast<cnet::Response*>(response)->Release();
  }
}

const char* CnetResponseInitialUrl(CnetResponse raw_response) {
  if (raw_response != NULL) {
    cnet::Response* response = static_cast<cnet::Response*>(raw_response);
    if (!response->initial_url().empty()) {
      return response->initial_url().c_str();
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

const char* CnetResponseOriginalUrl(CnetResponse raw_response) {
  if (raw_response != NULL) {
    cnet::Response* response = static_cast<cnet::Response*>(raw_response);
    if (!response->original_url().is_empty()) {
      return response->original_url().possibly_invalid_spec().c_str();
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

const char* CnetResponseFinalUrl(CnetResponse raw_response) {
  if (raw_response != NULL) {
    cnet::Response* response = static_cast<cnet::Response*>(raw_response);
    if (!response->final_url().is_empty()) {
      return response->final_url().possibly_invalid_spec().c_str();
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

const CnetLoadTiming* CnetResponseTiming(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->load_timing();
  } else {
    return NULL;
  }
}

const char* CnetResponseBody(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->response_body();
  } else {
    return NULL;
  }
}

int CnetResponseLength(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->response_length();
  } else {
    return 0;
  }
}

int CnetResponseSucceeded(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->status().status() ==
        net::URLRequestStatus::Status::SUCCESS;
  } else {
    return 0;
  }
}

int CnetResponseFailed(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->status().status() !=
        net::URLRequestStatus::Status::SUCCESS;
  } else {
    return 1;
  }
}

int CnetResponseCancelled(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->status().status() !=
        net::URLRequestStatus::Status::CANCELED;
  } else {
    return 0;
  }
}

int CnetResponseHttpCode(CnetResponse response) {
  if (response != NULL) {
    return static_cast<cnet::Response*>(response)->http_response_code();
  } else {
    return -1;
  }
}

char* CnetResponseFirstHeaderCopy(CnetResponse raw_response,
    const char* header_name) {
  char *header_value = NULL;
  if (raw_response != NULL) {
    cnet::Response* response = static_cast<cnet::Response*>(raw_response);
    scoped_refptr<net::HttpResponseHeaders> headers(response->response_headers());
    if (headers != NULL) {
      std::string value;
      if (headers->EnumerateHeader(NULL, header_name, &value)) {
        header_value = (char*)malloc(value.length() + 1);
        if (header_value != NULL) {
          memcpy(header_value, value.data(), value.length());
          header_value[value.length()] = '\0';
        }
      }
    }
  }
  return header_value;
}

};

