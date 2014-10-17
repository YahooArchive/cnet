// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/response_adapter.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_array.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/android/cnet_jni.h"
#include "yahoo/cnet/cnet.h"
#include "yahoo/cnet/cnet_response.h"

// Generated headers
#include "jni/Response_jni.h"

namespace cnet {
namespace android {

bool ResponseAdapterRegisterJni(JNIEnv* j_env) {
  // Register the generated JNI methods.
  return RegisterNativesImpl(j_env);
}

void InvokeResponseRelease(JNIEnv* j_env, jobject j_response) {
  Java_Response_release(j_env, j_response);
}

/* static */
base::android::ScopedJavaLocalRef<jobject> ResponseAdapter::CreateFromNative(
    JNIEnv* j_env, scoped_refptr<cnet::Response> response) {
  ResponseAdapter* response_adapter = new ResponseAdapter(response);
  return Java_Response_create(j_env, reinterpret_cast<jlong>(response_adapter));
}

ResponseAdapter::ResponseAdapter(scoped_refptr<cnet::Response> response)
    : response_(response) {
  DCHECK(response.get() != NULL);
}

ResponseAdapter::~ResponseAdapter() {
}

void ResponseAdapter::ReleaseResponseAdapter(JNIEnv* j_env, jobject j_caller) {
  delete this;
}

base::android::ScopedJavaLocalRef<jbyteArray> ResponseAdapter::GetBody(
    JNIEnv* j_env, jobject j_caller) {
  const char* body = response_->response_body();
  if (body == NULL) {
    return base::android::ScopedJavaLocalRef<jbyteArray>();
  } else {
    return base::android::ToJavaByteArray(j_env,
        (const uint8*)body, response_->response_length());
  }
}

jint ResponseAdapter::GetBodyLength(JNIEnv* j_env, jobject j_caller) {
  return response_->response_length();
}

jlong ResponseAdapter::GetBodyRawPointer(JNIEnv* j_env, jobject j_caller) {
  const char* body = response_->response_body();
  return reinterpret_cast<jlong>(body);
}

base::android::ScopedJavaLocalRef<jstring> ResponseAdapter::GetOriginalUrl(
    JNIEnv* j_env, jobject j_caller) {
  const GURL& url(response_->original_url());
  if (!url.is_empty()) {
    return base::android::ConvertUTF8ToJavaString(j_env,
        url.possibly_invalid_spec());
  } else {
    return base::android::ScopedJavaLocalRef<jstring>();
  }
}

base::android::ScopedJavaLocalRef<jstring> ResponseAdapter::GetFinalUrl(
    JNIEnv* j_env, jobject j_caller) {
  const GURL& url(response_->final_url());
  if (!url.is_empty()) {
    return base::android::ConvertUTF8ToJavaString(j_env,
        url.possibly_invalid_spec());
  } else {
    return base::android::ScopedJavaLocalRef<jstring>();
  }
}

jint ResponseAdapter::GetHttpResponseCode(JNIEnv* j_env, jobject j_caller) {
  return response_->http_response_code();
}

jint ResponseAdapter::GetNetError(JNIEnv* j_env, jobject j_caller) {
  return response_->status().error();
}

jboolean ResponseAdapter::WasCached(JNIEnv* j_env, jobject j_caller) {
  return response_->was_cached();
}

jboolean ResponseAdapter::WasFetchedViaProxy(JNIEnv* j_env, jobject j_caller) {
  return response_->was_fetched_via_proxy();
}

jboolean ResponseAdapter::WasFetchedViaSpdy(JNIEnv* j_env, jobject j_caller) {
  return response_->was_fetched_via_spdy();
}

jboolean ResponseAdapter::WasFetchedViaQuic(JNIEnv* j_env, jobject j_caller) {
  return response_->was_fetched_via_quic();
}


base::android::ScopedJavaLocalRef<jobjectArray>
ResponseAdapter::GetResponseHeader(
    JNIEnv* j_env, jobject j_caller, jstring j_header_name) {
  std::string header_name = base::android::ConvertJavaStringToUTF8(j_env,
      j_header_name);
  scoped_refptr<net::HttpResponseHeaders> headers(
      response_->response_headers());

  size_t value_count = 0;
  std::string value;
  void *value_iter = NULL;
  while (headers->EnumerateHeader(&value_iter, header_name, &value)) {
    value_count++;
  }

  jobjectArray joa = NULL;
  if (value_count > 0) {
    base::android::ScopedJavaLocalRef<jclass> string_clazz =
        base::android::GetClass(j_env, "java/lang/String");
    joa = j_env->NewObjectArray(value_count, string_clazz.obj(),
        NULL);
    base::android::CheckException(j_env);

    size_t extracted = 0;
    value_iter = NULL;
    while (headers->EnumerateHeader(&value_iter, header_name, &value) &&
           (extracted < value_count)) {
      base::android::ScopedJavaLocalRef<jstring> j_value =
          base::android::ConvertUTF8ToJavaString(j_env, value);
      j_env->SetObjectArrayElement(joa, extracted, j_value.obj());
      extracted++;
    }
  }

  return ScopedJavaLocalRef<jobjectArray>(j_env, joa);
}

} // namespace android
} // namespace cnet

