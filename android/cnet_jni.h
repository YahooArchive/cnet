// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_ANDROID_CNET_JNI_H_
#define YAHOO_CNET_ANDROID_CNET_JNI_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#define CNET_EXPORT __declspec(dllexport)
#else
#define CNET_EXPORT __attribute__((visibility("default")))
#endif

// Register the Android application context with Cnet.  This is required.
CNET_EXPORT void CnetJniInitializeAppContext(JNIEnv* env, jobject context);

#ifdef __cplusplus
};
#endif

#endif  // YAHOO_CNET_ANDROID_CNET_JNI_H_
