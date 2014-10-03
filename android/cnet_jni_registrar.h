// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_CNET_JNI_REGISTRAR_H_
#define YAHOO_CNET_ANDROID_CNET_JNI_REGISTRAR_H_

#include <jni.h>

#include "base/base_export.h"

namespace cnet {
namespace android {

// Register all JNI bindings for cnet.
BASE_EXPORT bool RegisterJni(JNIEnv* env);

} // namespace android
} // namespace cnet

#endif  // YAHOO_CNET_ANDROID_CNET_JNI_REGISTRAR_H_

