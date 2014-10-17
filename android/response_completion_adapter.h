// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSECOMPLETION_H_
#define YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSECOMPLETION_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace cnet {
namespace android {

bool ResponseCompletionRegisterJni(JNIEnv* j_env);

jboolean InvokeFetcherResponseCompletion(JNIEnv* j_env, jobject j_completion,
    jobject j_fetcher, jobject j_response);

} // namespace android
} // namespace cnet

#endif //  YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSECOMPLETION_H_
