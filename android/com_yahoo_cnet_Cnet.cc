// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/com_yahoo_cnet_Cnet.h"

#include "yahoo/cnet/android/cnet_jni.h"
#include "yahoo/cnet/cnet.h"

// Generated headers
#include "jni/Cnet_jni.h"

namespace cnet {
namespace android {

bool CnetAdapterRegisterJni(JNIEnv* j_env) {
  // Register the generated JNI methods.
  return RegisterNativesImpl(j_env);
}

static void InitLibraryOnUiThread(JNIEnv* j_env, jclass j_class,
    jobject j_context) {
  CnetJniInitializeAppContext(j_env, j_context);

  // Attach to the Android UI loop.
  CnetMessageLoopForUiGet();
}

} // namespace android
} // namespace cnet

