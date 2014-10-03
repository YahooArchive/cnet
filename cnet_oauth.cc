// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Changes to this code are Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/cnet_oauth.h"

#include "base/base64.h"
#include "base/format_macros.h"
#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "crypto/hmac.h"
#include "url/gurl.h"
#include "yahoo/cnet/cnet_url_params.h"

namespace {

const int kHexBase = 16;
char kHexDigits[] = "0123456789ABCDEF";
const size_t kHmacDigestLength = 20;
const int kMaxNonceLength = 30;
const int kMinNonceLength = 15;

const char kOAuthNonceCharacters[] =
    "abcdefghijklmnopqrstuvwyz"
    "ABCDEFGHIJKLMNOPQRSTUVWYZ"
    "0123456789_";

std::string Encode(const std::string& text) {
  std::string result;
  std::string::const_iterator cursor;
  std::string::const_iterator limit;
  for (limit = text.end(), cursor = text.begin(); cursor != limit; ++cursor) {
    char character = *cursor;
    if (IsAsciiAlpha(character) || IsAsciiDigit(character)) {
      result += character;
    } else {
      switch (character) {
        case '-':
        case '.':
        case '_':
        case '~':
          result += character;
          break;
        default:
          unsigned char byte = static_cast<unsigned char>(character);
          result = result + '%' + kHexDigits[byte / kHexBase] +
          kHexDigits[byte % kHexBase];
      }
    }
  }
  return result;
}

std::string GenerateNonce() {
  char result[kMaxNonceLength + 1];
  int length = base::RandUint64() % (kMaxNonceLength - kMinNonceLength + 1) +
      kMinNonceLength;
  result[length] = '\0';
  for (int index = 0; index < length; ++index) {
    result[index] = kOAuthNonceCharacters[
        base::RandUint64() % (sizeof(kOAuthNonceCharacters) - 1)];
  }
  return result;
}
  
std::string GenerateTimestamp() {
  return base::StringPrintf(
      "%" PRId64,
      (base::Time::NowFromSystemTime() - base::Time::UnixEpoch()).InSeconds());
}
 
std::string BuildBaseStringParameters(const cnet::UrlParams& params) {
  std::string result;
  cnet::UrlParams::const_iterator cursor;
  cnet::UrlParams::const_iterator limit;
  bool first = true;
  for (cursor = params.begin(), limit = params.end();
       cursor != limit;
       ++cursor) {
    if (first) {
      first = false;
    } else {
      result += '&';
    }
    result += Encode(cursor->first);
    result += '=';
    result += Encode(cursor->second);
  }
  return result;
}
  
std::string BuildBaseString(const GURL& request_base_url,
    const std::string& http_method, const std::string& base_parameters) {
  return base::StringPrintf("%s&%s&%s", http_method.c_str(),
                            Encode(request_base_url.spec()).c_str(),
                            Encode(base_parameters).c_str());
}

bool SignHmacSha1(const std::string& text, const std::string& key,
    std::string* signature_return) {
  crypto::HMAC hmac(crypto::HMAC::SHA1);
  DCHECK(hmac.DigestLength() == kHmacDigestLength);
  unsigned char digest[kHmacDigestLength];
  bool result = hmac.Init(key) &&
      hmac.Sign(text, digest, kHmacDigestLength);
  if (result) {
    base::Base64Encode(
        std::string(reinterpret_cast<const char*>(digest), kHmacDigestLength),
        signature_return);
  }
  return result;
}

} // namespace

namespace cnet {

OauthCredentials::OauthCredentials() {
}

OauthCredentials::~OauthCredentials() {
}

void OauthSignRequest(const OauthCredentials& credentials,
                      const GURL& request_url,
                      const std::string& http_method,
                      UrlParams& params) {
  params["oauth_nonce"] = GenerateNonce();
  params["oauth_timestamp"] = GenerateTimestamp();
  params["oauth_consumer_key"] = credentials.app_key;
  params["oauth_token"] = credentials.token;
  params["oauth_version"] = "1.0";
  params["oauth_signature_method"] = "HMAC-SHA1";
  
  std::string base_parameters = BuildBaseStringParameters(params);
  std::string base = BuildBaseString(request_url, http_method, base_parameters);
  std::string key = credentials.app_secret + '&' + credentials.token_secret;
  std::string signature;
  if (SignHmacSha1(base, key, &signature)) {
    params["oauth_signature"] = signature;
  }
}

std::string OauthCompatibleEncodeParams(const UrlParams& params) {
  return BuildBaseStringParameters(params);
}

} // namespace cnet

