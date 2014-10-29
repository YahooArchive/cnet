// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_COM_YAHOO_CNET_POOL_H_
#define YAHOO_CNET_ANDROID_COM_YAHOO_CNET_POOL_H_

#include <jni.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace cnet {
class Pool;

namespace android {

bool PoolAdapterRegisterJni(JNIEnv* j_env);

class PoolAdapter {
 public:
  PoolAdapter(scoped_refptr<cnet::Pool> pool);
  ~PoolAdapter();

  scoped_refptr<cnet::Pool> pool() { return pool_; }

  jlong CreateFetcherAdapter(JNIEnv* j_env, jobject j_caller,
      jstring j_url, jstring j_method, jobject j_completion);

  void ReleasePoolAdapter(JNIEnv* j_env, jobject j_caller);

  void SetProxyRules(JNIEnv* j_env, jobject j_caller, jstring j_rules);

  void SetTrustAllCertAuthorities(JNIEnv* j_env, jobject j_caller,
      jboolean j_value);
  jboolean GetTrustAllCertAuthorities(JNIEnv* j_env, jobject j_caller);

  void SetEnableSslFalseStart(JNIEnv* j_env, jobject j_caller,
      jboolean j_value);
  jboolean GetEnableSslFalseStart(JNIEnv* j_env, jobject j_caller);

  void AddQuicHint(JNIEnv* j_env, jobject j_caller, jstring j_host,
      jint j_port, jint j_alternate_port);

  void Preconnect(JNIEnv* j_env, jobject j_caller, jstring j_url,
      jint j_num_streams);

 private:
  scoped_refptr<cnet::Pool> pool_;

  DISALLOW_COPY_AND_ASSIGN(PoolAdapter);
};

} // namespace android
} // namespace cnet

#endif //  YAHOO_CNET_ANDROID_COM_YAHOO_CNET_POOL_H_
