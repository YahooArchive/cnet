// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Changes to this code are Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_MIME_H_
#define YAHOO_CNET_CNET_MIME_H_

#include <string>

namespace cnet {
namespace mime {

std::string GenerateMimeBoundary();
void AddMultipartValueForPost(const std::string& value_name,
    const std::string& value, const std::string& mime_boundary,
    std::string* post_data);
void StartMultipartValueForPost(const std::string& value_name,
    const std::string& extra_name, const std::string& extra,
    const std::string& content_type,
    const std::string& mime_boundary, std::string* post_data);
void FinishMultipartValueForPost(std::string* post_data);
void AddMultipartFinalDelimiterForPost(const std::string& mime_boundary,
    std::string* post_data);

} // namespace mime
} // namespace cnet

#endif  // YAHOO_CNET_CNET_MIME_H_
