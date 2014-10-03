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
#include "yahoo/cnet/android/com_yahoo_cnet_Cnet.h"
#include "yahoo/cnet/android/com_yahoo_cnet_Fetcher.h"
#include "yahoo/cnet/android/com_yahoo_cnet_Pool.h"
#include "yahoo/cnet/android/com_yahoo_cnet_Response.h"
#include "yahoo/cnet/android/com_yahoo_cnet_ResponseCompletion.h"

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

