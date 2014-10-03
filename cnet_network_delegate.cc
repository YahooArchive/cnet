// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Changes to this code are Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "yahoo/cnet/cnet_network_delegate.h"

#include "net/base/net_errors.h"

namespace cnet {

CnetNetworkDelegate::CnetNetworkDelegate() {
}

CnetNetworkDelegate::~CnetNetworkDelegate() {
}

int CnetNetworkDelegate::OnBeforeURLRequest(net::URLRequest* request,
    const net::CompletionCallback& callback, GURL* new_url) {
  return net::OK;
}

int CnetNetworkDelegate::OnBeforeSendHeaders(net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

int CnetNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* _response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return net::OK;
}

net::NetworkDelegate::AuthRequiredResponse
CnetNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  return net::NetworkDelegate::AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool CnetNetworkDelegate::OnCanGetCookies(const net::URLRequest& request,
    const net::CookieList& cookie_list) {
  return false;
}

bool CnetNetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
    const std::string& cookie_line, net::CookieOptions* options) {
  return false;
}

bool CnetNetworkDelegate::OnCanAccessFile(const net::URLRequest& request,
    const base::FilePath& path) const {
  return false;
}

bool CnetNetworkDelegate::OnCanThrottleRequest(const net::URLRequest& request) const {
  return false;
}

int CnetNetworkDelegate::OnBeforeSocketStreamConnect( net::SocketStream* stream,
    const net::CompletionCallback& callback) {
  return net::OK;
}

} // namespace cnet
