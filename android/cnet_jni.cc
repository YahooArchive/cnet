// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <jni.h>

#include "yahoo/cnet/android/cnet_jni.h"

#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "net/android/net_jni_registrar.h"
#include "url/android/url_jni_registrar.h"
#include "yahoo/cnet/cnet.h"
#include "yahoo/cnet/android/cnet_jni.h"
#include "yahoo/cnet/android/cnet_jni_registrar.h"

namespace {

const base::android::RegistrationMethod kCnetRegisteredMethods[] = {
  {"BaseAndroid", base::android::RegisterJni},
  {"NetAndroid", net::android::RegisterJni},
  {"UrlAndroid", url::android::RegisterJni},
  {"CnetAndroid", cnet::android::RegisterJni},
};

}

extern "C" {

jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  if (base::android::IsVMInitialized()) {
    LOG(ERROR) << "Double load of the cnet library.";
  } else {
    LOG(INFO) << "Loading Cnet.";

    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
      return -1;
    }

    base::android::InitVM(vm);

    if (!base::android::RegisterNativeMethods(
          env, kCnetRegisteredMethods, arraysize(kCnetRegisteredMethods))) {
      LOG(ERROR) << "Failed to register Cnet native methods.";
      return -1;
    }

    CnetInitialize(0);
  }

  return JNI_VERSION_1_6;
}

void JNIEXPORT JNICALL JNI_OnUnload(JavaVM* jvm, void* reserved) {
  LOG(INFO) << "Unloading Cnet";
  CnetCleanup();
}

void CnetJniInitializeAppContext(JNIEnv* env, jobject context) {
  base::android::ScopedJavaLocalRef<jobject> scoped_context(env, context);
  base::android::InitApplicationContext(env, scoped_context);
}

};
