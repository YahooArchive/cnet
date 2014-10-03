// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Changes to this code are Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/cnet_mime.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"

namespace cnet {
namespace mime {

std::string GenerateMimeBoundary() {
  std::string mime_boundary;
  int r1 = base::RandInt(0, kint32max);
  int r2 = base::RandInt(0, kint32max);
  base::SStringPrintf(&mime_boundary,
    "---------------------------%08X%08X", r1, r2);
  return mime_boundary;
}

void AddMultipartValueForPost(const std::string& value_name,
    const std::string& value, const std::string& mime_boundary,
    std::string* post_data) {
  DCHECK(post_data);
  // First line is the boundary.
  post_data->append("--" + mime_boundary + "\r\n");
  // Next line is the Content-disposition.
  post_data->append("Content-Disposition: form-data; name=\"" +
                    value_name + "\"\r\n");
  // Leave an empty line and append the value.
  post_data->append("\r\n" + value + "\r\n");
}

void StartMultipartValueForPost(const std::string& value_name,
    const std::string& extra_name, const std::string& extra,
    const std::string& content_type,
    const std::string& mime_boundary, std::string* post_data) {
  DCHECK(post_data);
  // First line is the boundary.
  post_data->append("--" + mime_boundary + "\r\n");
  // Next line is the Content-disposition.
  post_data->append("Content-Disposition: form-data; name=\"" +
                    value_name + "\"; " + extra_name + "=\"" + extra + "\"\r\n");
  if (!content_type.empty()) {
    post_data->append("Content-Type: " + content_type + "\r\n");
  }
  // Leave an empty line.
  post_data->append("\r\n");
}

void FinishMultipartValueForPost(std::string* post_data) {
  // Terminate the value in the multi-part.
  post_data->append("\r\n");
}

void AddMultipartFinalDelimiterForPost(const std::string& mime_boundary,
    std::string* post_data) {
  DCHECK(post_data);
  post_data->append("--" + mime_boundary + "--\r\n");
}

} // namespace mime
} // namespace cnet

