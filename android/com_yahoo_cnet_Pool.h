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
  static PoolAdapter* PoolAdapterFromObject(JNIEnv* j_env, jobject j_object);

  PoolAdapter(scoped_refptr<cnet::Pool> pool);
  ~PoolAdapter();

  scoped_refptr<cnet::Pool> pool() { return pool_; }

  void ReleasePoolAdapter(JNIEnv* j_env, jobject j_caller);

  void SetProxyRules(JNIEnv* j_env, jobject j_caller, jstring j_rules);

  void SetTrustAllCertAuthorities(JNIEnv* j_env, jobject j_caller,
      jboolean j_value);
  jboolean GetTrustAllCertAuthorities(JNIEnv* j_env, jobject j_caller);

  void SetEnableSslFalseStart(JNIEnv* j_env, jobject j_caller,
      jboolean j_value);
  jboolean GetEnableSslFalseStart(JNIEnv* j_env, jobject j_caller);

 private:
  scoped_refptr<cnet::Pool> pool_;

  DISALLOW_COPY_AND_ASSIGN(PoolAdapter);
};

} // namespace android
} // namespace cnet

#endif //  YAHOO_CNET_ANDROID_COM_YAHOO_CNET_POOL_H_
