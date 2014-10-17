// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/fetcher_adapter.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/android/pool_adapter.h"
#include "yahoo/cnet/android/response_adapter.h"
#include "yahoo/cnet/android/response_completion_adapter.h"
#include "yahoo/cnet/android/cnet_jni.h"
#include "yahoo/cnet/cnet_fetcher.h"
#include "yahoo/cnet/cnet_pool.h"
#include "yahoo/cnet/cnet_oauth.h"
#include "yahoo/cnet/cnet_response.h"

// Generated headers
#include "jni/Fetcher_jni.h"

namespace cnet {
namespace android {

bool FetcherAdapterRegisterJni(JNIEnv* j_env) {
  // Register the generated JNI methods.
  return RegisterNativesImpl(j_env);
}

void InvokeFetcherRelease(JNIEnv* j_env, jobject j_fetcher) {
  Java_Fetcher_release(j_env, j_fetcher);
}

jlong CreateFetcherAdapter(JNIEnv* j_env, jobject j_caller, jobject j_pool,
    jstring j_url, jstring j_method, jobject j_completion) {
  PoolAdapter* pool_adapter = PoolAdapter::PoolAdapterFromObject(j_env, j_pool);
  std::string url = base::android::ConvertJavaStringToUTF8(j_env, j_url);
  std::string method = base::android::ConvertJavaStringToUTF8(j_env, j_method);

  // Retain the Java fetcher object to ensure that the fetch
  // completes even if the fetcher object goes out of scope.
  base::android::ScopedJavaLocalRef<jobject> local_fetcher(j_env,
      j_caller);
  base::android::ScopedJavaGlobalRef<jobject> global_fetcher(
      local_fetcher);

  base::android::ScopedJavaLocalRef<jobject> local_completion(j_env,
      j_completion);
  base::android::ScopedJavaGlobalRef<jobject> global_completion(
      local_completion);

  cnet::Fetcher::CompletionCallback completion = base::Bind(
      &FetcherAdapter::InvokeCompletion, global_completion.Release(),
      global_fetcher.Release());
  cnet::Fetcher::ProgressCallback download_callback;
  cnet::Fetcher::ProgressCallback upload_callback;

  scoped_refptr<cnet::Fetcher> fetcher(new cnet::Fetcher(pool_adapter->pool(), 
      url, method, completion, download_callback, upload_callback));

  FetcherAdapter* fetcher_adapter = new FetcherAdapter(fetcher);
  return reinterpret_cast<jlong>(fetcher_adapter);
}

FetcherAdapter::FetcherAdapter(scoped_refptr<cnet::Fetcher> fetcher)
    : fetcher_(fetcher) {
}

FetcherAdapter::~FetcherAdapter() {
}

void FetcherAdapter::ReleaseFetcherAdapter(JNIEnv* j_env, jobject j_caller) {
  delete this;
}

void FetcherAdapter::Start(JNIEnv* j_env, jobject j_caller) {
  fetcher_->Start();
}

void FetcherAdapter::Cancel(JNIEnv* j_env, jobject j_caller) {
  fetcher_->Cancel();
}

void FetcherAdapter::SetCacheBehavior(JNIEnv* j_env, jobject j_caller,
    jint j_behavior) {
  fetcher_->SetCacheBehavior(
      static_cast<cnet::Fetcher::CacheBehavior>(j_behavior));
}

void FetcherAdapter::SetHeader(JNIEnv* j_env, jobject j_caller, jstring j_key,
    jstring j_value) {
  std::string key = base::android::ConvertJavaStringToUTF8(j_env, j_key);
  std::string value = base::android::ConvertJavaStringToUTF8(j_env, j_value);
  fetcher_->SetHeader(key, value);
}

void FetcherAdapter::SetOauthCredentials(JNIEnv* j_env, jobject j_caller,
    jstring j_app_key, jstring j_app_secret, jstring j_token,
    jstring j_token_secret) {
  cnet::OauthCredentials creds;
  creds.app_key = base::android::ConvertJavaStringToUTF8(j_env, j_app_key);
  creds.app_secret = base::android::ConvertJavaStringToUTF8(j_env,
      j_app_secret);
  creds.token = base::android::ConvertJavaStringToUTF8(j_env, j_token);
  creds.token_secret = base::android::ConvertJavaStringToUTF8(j_env,
      j_token_secret);
  fetcher_->SetOauthCredentials(creds);
}

void FetcherAdapter::SetUrlParamsEncoding(JNIEnv* j_env, jobject j_caller,
    jint j_encoding) {
  fetcher_->SetUrlParamsEncoding(
      static_cast<cnet::Fetcher::UrlParamsEncoding>(j_encoding));
}

void FetcherAdapter::SetUrlParam(JNIEnv* j_env, jobject j_caller, jstring j_key,
    jstring j_value) {
  std::string key = base::android::ConvertJavaStringToUTF8(j_env, j_key);
  std::string value = base::android::ConvertJavaStringToUTF8(j_env, j_value);
  fetcher_->SetUrlParam(key, value);
}

void FetcherAdapter::SetUrlParamFile(JNIEnv* j_env, jobject j_caller,
    jstring j_key, jstring j_filename, jstring j_content_type, jstring j_path,
    jlong j_range_offset, jlong j_range_length) {
  std::string key = base::android::ConvertJavaStringToUTF8(j_env, j_key);
  std::string filename = base::android::ConvertJavaStringToUTF8(j_env,
      j_filename);
  std::string content_type = base::android::ConvertJavaStringToUTF8(j_env,
      j_content_type);
  base::FilePath path(base::android::ConvertJavaStringToUTF8(j_env, j_path));
  fetcher_->SetUrlParamFile(key, filename, content_type, path, 
      j_range_offset, j_range_length);
}

void FetcherAdapter::SetUploadStringBody(JNIEnv* j_env, jobject j_caller,
    jstring j_content_type, jstring j_body) {
  std::string content_type = base::android::ConvertJavaStringToUTF8(j_env,
      j_content_type);
  std::string body = base::android::ConvertJavaStringToUTF8(j_env, j_body);
  fetcher_->SetUploadBody(content_type, body);
}

void FetcherAdapter::SetUploadByteBody(JNIEnv* j_env, jobject j_caller,
    jstring j_content_type, jbyteArray j_body) {
  std::string content_type = base::android::ConvertJavaStringToUTF8(j_env,
      j_content_type);
  std::string body;
  if (j_body != NULL) {
    jsize len = j_env->GetArrayLength(j_body);
    jbyte* bytes = j_env->GetByteArrayElements(j_body, NULL);
    body = std::string(reinterpret_cast<char*>(bytes),
        static_cast<size_t>(len));
    j_env->ReleaseByteArrayElements(j_body, bytes, JNI_ABORT);
  }
  fetcher_->SetUploadBody(content_type, body);
}

void FetcherAdapter::SetUploadFilePath(JNIEnv* j_env, jobject j_caller,
    jstring j_content_type, jstring j_path,
    jlong j_range_offset, jlong j_range_length) {
  std::string content_type = base::android::ConvertJavaStringToUTF8(j_env,
      j_content_type);
  base::FilePath path(base::android::ConvertJavaStringToUTF8(j_env, j_path));
  fetcher_->SetUploadFilePath(content_type, path, j_range_offset,
      j_range_length);
}

/* static */
void FetcherAdapter::InvokeCompletion(
    jobject j_completion_global,
    jobject j_fetcher_global,
    scoped_refptr<cnet::Fetcher> fetcher,
    scoped_refptr<cnet::Response> response) {
  // We are on a background thread.
  JNIEnv* j_env = base::android::AttachCurrentThread();
  {
    base::android::ScopedJavaLocalRef<jobject> response_local;
    if (response != NULL) {
      response_local = ResponseAdapter::CreateFromNative(j_env, response);
    }

    jboolean release_now = InvokeFetcherResponseCompletion(j_env,
        j_completion_global, j_fetcher_global, response_local.obj());
    base::android::ClearException(j_env);

    if (release_now) {
      InvokeFetcherRelease(j_env, j_fetcher_global);
      base::android::ClearException(j_env);

      InvokeResponseRelease(j_env, response_local.obj());
      base::android::ClearException(j_env);
    }

    j_env->DeleteGlobalRef(j_completion_global);
    j_env->DeleteGlobalRef(j_fetcher_global);
  }
  base::android::DetachFromVM();
}

} // namespace android
} // namespace cnet

