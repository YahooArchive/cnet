// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_CNET_YMAGINE_H_
#define YAHOO_CNET_ANDROID_CNET_YMAGINE_H_

namespace cnet {
namespace ymagine {
namespace android {

class Vbitmap;

typedef int (*YmagineSNI_Decode)(Vbitmap* bitmap, const char* data,
    int data_len, int max_width, int max_height, int scale_mode);
typedef Vbitmap* (*VbitmapInitAndroid)(JNIEnv* j_env, jobject j_bitmap);
typedef jobject (*VbitmapGetAndroid)(Vbitmap* vbitmap);
typedef int (*VbitmapRelease)(Vbitmap* vbitmap);

struct YmagineSyms {
  YmagineSNI_Decode decode;
  VbitmapInitAndroid vbitmapInitAndroid;
  VbitmapGetAndroid vbitmapGetAndroid;
  VbitmapRelease vbitmapRelease;
};

void SymsInit();
const YmagineSyms* get_syms();

} // namespace android
} // namespace ymagine
} // namespace cnet

#endif // YAHOO_CNET_ANDROID_CNET_YMAGINE_H_

