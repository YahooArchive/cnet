// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/response_completion_adapter.h"

// Generated headers
#include "jni/ResponseCompletion_jni.h"

namespace cnet {
namespace android {

bool ResponseCompletionRegisterJni(JNIEnv* j_env) {
  // Register the generated JNI methods.
  return RegisterNativesImpl(j_env);
}

jboolean InvokeFetcherResponseCompletion(JNIEnv* j_env, jobject j_completion,
    jobject j_fetcher, jobject j_response) {
  return Java_ResponseCompletion_onBackgroundComplete(j_env, j_completion,
      j_fetcher, j_response);
}

} // namespace android
} // namespace cnet

