// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/cnet_response.h"

#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/cnet.h"

namespace cnet {

Response::Response(const std::string& initial_url,
    const GURL& original_url, const GURL& final_url,
    scoped_refptr<net::GrowableIOBuffer> read_buffer,
    const UrlParams& url_params, scoped_ptr<CnetLoadTiming> load_timing,
    const net::URLRequestStatus& status, int http_response_code,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    scoped_ptr<net::HttpResponseInfo> response_info)
    : initial_url_(initial_url),
      original_url_(original_url), final_url_(final_url),
      read_buffer_(read_buffer), url_params_(url_params),
      timing_(load_timing.Pass()), status_(status),
      http_response_code_(http_response_code),
      response_headers_(response_headers),
      response_info_(response_info.Pass()) {
}

Response::~Response() {
}

const char* Response::response_body() {
  if (read_buffer_.get() == NULL) {
    return NULL;
  } else {
    return read_buffer_->StartOfBuffer();
  }
}

int Response::response_length() {
  if (read_buffer_.get() == NULL) {
    return 0;
  } else {
    return read_buffer_->offset();
  }
}

} // namespace cnet
