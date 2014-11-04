// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/response_adapter.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_array.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/android/cnet_jni.h"
#include "yahoo/cnet/android/cnet_ymagine.h"
#include "yahoo/cnet/cnet.h"
#include "yahoo/cnet/cnet_response.h"

// Generated headers
#include "jni/CnetResponse_jni.h"

namespace cnet {
namespace android {

bool ResponseAdapterRegisterJni(JNIEnv* j_env) {
  // Register the generated JNI methods.
  return RegisterNativesImpl(j_env);
}

void InvokeResponseRelease(JNIEnv* j_env, jobject j_response) {
  Java_CnetResponse_release(j_env, j_response);
}

static jboolean HasNativeBitmap(JNIEnv* j_env, jobject j_caller) {
  return cnet::ymagine::android::get_syms() != NULL;
}

/* static */
base::android::ScopedJavaLocalRef<jobject> ResponseAdapter::CreateFromNative(
    JNIEnv* j_env, scoped_refptr<cnet::Response> response) {
  ResponseAdapter* response_adapter = new ResponseAdapter(response);
  return Java_CnetResponse_create(j_env, reinterpret_cast<jlong>(
      response_adapter));
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

jboolean ResponseAdapter::GetBodyAsBitmap(JNIEnv* j_env, jobject j_caller,
    jobject j_recycled_bitmap,
    jint j_max_width, jint j_max_height, jint j_scale_type) {
  jboolean result = false;
  const char* body = response_->response_body();
  int body_len = response_->response_length();
  const cnet::ymagine::android::YmagineSyms *syms =
      cnet::ymagine::android::get_syms();
  if ((syms != NULL) && (body != NULL) && (body_len > 0)) {
    cnet::ymagine::android::Vbitmap* vbitmap = syms->vbitmapInitAndroid(j_env,
        j_recycled_bitmap);
    if (vbitmap != NULL) {
      result = -1 != syms->decode(vbitmap, body, body_len,
                                  j_max_width, j_max_height, j_scale_type);
      syms->vbitmapRelease(vbitmap);
    }
  }
  return result;
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

