// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Changes to this code are Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_OAUTH_H_
#define YAHOO_CNET_CNET_OAUTH_H_

#include <string>

#include "yahoo/cnet/cnet_url_params.h"

class GURL;

namespace cnet {

class OauthCredentials {
public:
  OauthCredentials();
  ~OauthCredentials();

  std::string app_key;
  std::string app_secret;
  std::string token;
  std::string token_secret;
};

void OauthSignRequest(
    const OauthCredentials& credentials,
    const GURL& request_url,
    const std::string& http_method,
    UrlParams& params /* in/out */);

std::string OauthCompatibleEncodeParams(const UrlParams& params);

} // namespace cnet

#endif  //YAHOO_CNET_CNET_OAUTH_H_
