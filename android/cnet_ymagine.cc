// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <stdlib.h>
#include <dlfcn.h>
#include <jni.h>

#include "base/logging.h"
#include "yahoo/cnet/android/cnet_ymagine.h"

namespace cnet {
namespace ymagine {
namespace android {

YmagineSyms* g_syms = NULL;

void SymsInit() {
  if (g_syms == NULL) {
    YmagineSyms syms;
    syms.decode = reinterpret_cast<YmagineSNI_Decode>(
        dlsym(RTLD_DEFAULT, "YmagineSNI_Decode"));
    syms.vbitmapInitAndroid = reinterpret_cast<VbitmapInitAndroid>(
        dlsym(RTLD_DEFAULT, "VbitmapInitAndroid"));
    syms.vbitmapGetAndroid = reinterpret_cast<VbitmapGetAndroid>(
        dlsym(RTLD_DEFAULT, "VbitmapGetAndroid"));
    syms.vbitmapRelease = reinterpret_cast<VbitmapRelease>(
        dlsym(RTLD_DEFAULT, "VbitmapRelease"));

    if ((syms.decode != NULL) &&
        (syms.vbitmapInitAndroid != NULL) &&
        (syms.vbitmapGetAndroid != NULL) &&
        (syms.vbitmapRelease != NULL)) {
      LOG(INFO) << "Found Ymagine.";
      g_syms = new YmagineSyms(syms);
    }
  }
}

const YmagineSyms* get_syms() {
  return g_syms;
}

} // namespace android
} // namespace ymagine
} // namespace cnet

