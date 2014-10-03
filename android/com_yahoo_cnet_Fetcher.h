// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_COM_YAHOO_CNET_FETCHER_H_
#define YAHOO_CNET_ANDROID_COM_YAHOO_CNET_FETCHER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace cnet {
class Fetcher;
class Response;

namespace android {

bool FetcherAdapterRegisterJni(JNIEnv* j_env);

void InvokeFetcherRelease(JNIEnv* env, jobject j_fetcher);

class FetcherAdapter {
 public:
  FetcherAdapter(scoped_refptr<cnet::Fetcher> fetcher);
  ~FetcherAdapter();

  void ReleaseFetcherAdapter(JNIEnv* j_env, jobject j_caller);

  void Start(JNIEnv* j_env, jobject j_caller);
  void Cancel(JNIEnv* j_env, jobject j_caller);

  void SetCacheBehavior(JNIEnv* j_env, jobject j_caller, jint j_behavior);

  void SetHeader(JNIEnv* j_env, jobject j_caller, jstring j_key, jstring j_value);

  void SetOauthCredentials(JNIEnv* j_env, jobject j_caller, jstring j_app_key,
      jstring j_app_secret, jstring j_token, jstring j_token_secret);

  void SetUrlParamsEncoding(JNIEnv* j_env, jobject j_caller, jint j_encoding);
  void SetUrlParam(JNIEnv* j_env, jobject j_caller, jstring j_key,
      jstring j_value);
  void SetUrlParamFile(JNIEnv* j_env, jobject j_caller, jstring j_key,
      jstring j_filename, jstring j_content_type, jstring j_path,
      jlong j_range_offset, jlong j_range_length);

  void SetUploadStringBody(JNIEnv* j_env, jobject j_caller, jstring j_content_type,
      jstring j_body);
  void SetUploadByteBody(JNIEnv* j_env, jobject j_caller, jstring j_content_type,
      jbyteArray j_body);
  void SetUploadFilePath(JNIEnv* j_env, jobject j_caller,
      jstring j_content_type, jstring j_path, jlong j_range_offset,
      jlong j_range_length);

  static void InvokeCompletion(
      jobject j_completion_global,
      jobject j_fetcher_global,
      scoped_refptr<cnet::Fetcher> fetcher,
      scoped_refptr<cnet::Response> response);

 private:
  scoped_refptr<cnet::Fetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(FetcherAdapter);
};

} // namespace android
} // namespace cnet

#endif //  YAHOO_CNET_ANDROID_COM_YAHOO_CNET_FETCHER_H_
