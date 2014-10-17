// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/android/cnet_jni_registrar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "net/http/http_response_headers.h"
#include "yahoo/cnet/cnet_fetcher.h"
#include "yahoo/cnet/cnet_pool.h"
#include "yahoo/cnet/cnet_response.h"
#include "yahoo/cnet/android/cnet_adapter.h"
#include "yahoo/cnet/android/fetcher_adapter.h"
#include "yahoo/cnet/android/pool_adapter.h"
#include "yahoo/cnet/android/response_adapter.h"
#include "yahoo/cnet/android/response_completion_adapter.h"

namespace cnet {
namespace android { 

static base::android::RegistrationMethod kCnetRegisterMethods[] = {
  { "Cnet", CnetAdapterRegisterJni },
  { "Fetcher", FetcherAdapterRegisterJni },
  { "Pool", PoolAdapterRegisterJni },
  { "Response", ResponseAdapterRegisterJni },
  { "ResponseCompletion", ResponseCompletionRegisterJni },
};

bool RegisterJni(JNIEnv* env) {
  return base::android::RegisterNativeMethods(env, kCnetRegisterMethods,
      arraysize(kCnetRegisterMethods));
}

} // namespace android
} // namespace cnet

