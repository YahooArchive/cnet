// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import android.content.Context;

import org.chromium.base.JNINamespace;

@JNINamespace("cnet::android")
public class Cnet {
    private static final String[] NATIVE_LIBS = {"cnet"};

    /**
     * Load the native libraries.
     * This works around bugs in the native-library loading support
     * in several Android releases.
     */
    public static void loadLibraries(Context context) throws UnsatisfiedLinkError {
        LibraryLoader.loadLibraries(context, false, NATIVE_LIBS);
    }

    /**
     * Initialize the native Cnet library.
     * The Cnet library must be initialized prior to use.
     * This must execute on the Android UI/main thread.
     */
    public static void initLibraryOnUiThread(Context context) {
        nativeInitLibraryOnUiThread(context.getApplicationContext());
    }

    private static native void nativeInitLibraryOnUiThread(Context context);
}

