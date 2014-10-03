// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSE_H_
#define YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSE_H_

#include <jni.h>

#include "base/macros.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"

namespace cnet {
class Response;

namespace android {

bool ResponseAdapterRegisterJni(JNIEnv* j_env);

void InvokeResponseRelease(JNIEnv* j_env, jobject j_response);

class ResponseAdapter {
 public:
  ResponseAdapter(scoped_refptr<cnet::Response> response);
  ~ResponseAdapter();

  static base::android::ScopedJavaLocalRef<jobject> CreateFromNative(
          JNIEnv* j_env, scoped_refptr<cnet::Response> response);

  void ReleaseResponseAdapter(JNIEnv* j_env, jobject j_caller);

  base::android::ScopedJavaLocalRef<jbyteArray> GetBody(JNIEnv* j_env,
      jobject j_caller);
  jint GetBodyLength(JNIEnv* j_env, jobject j_caller);

  jlong GetBodyRawPointer(JNIEnv* j_env, jobject j_caller);

  base::android::ScopedJavaLocalRef<jstring> GetOriginalUrl(JNIEnv* j_env,
      jobject j_caller);
  base::android::ScopedJavaLocalRef<jstring> GetFinalUrl(JNIEnv* j_env,
      jobject j_caller);
  jint GetHttpResponseCode(JNIEnv* j_env, jobject j_caller);
  jint GetNetError(JNIEnv* j_env, jobject j_caller);
  base::android::ScopedJavaLocalRef<jobjectArray> GetResponseHeader(
          JNIEnv* j_env, jobject j_caller, jstring j_header_name);

 private:
  scoped_refptr<cnet::Response> response_;

  DISALLOW_COPY_AND_ASSIGN(ResponseAdapter);
};

} // namespace android
} // namespace cnet

#endif //  YAHOO_CNET_ANDROID_COM_YAHOO_CNET_RESPONSE_H_
