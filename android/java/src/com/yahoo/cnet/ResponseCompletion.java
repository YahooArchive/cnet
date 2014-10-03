// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

@JNINamespace("cnet::android")
public interface ResponseCompletion {
    public static final boolean RELEASE_NOW = true;
    public static final boolean RELEASE_LATER = false;

    /**
     * Invoked when a fetch has completed.
     * This is called on a work thread shared by all fetchers in a Cnet pool.
     * @param fetcher The fetcher that has completed.
     * @param response The response state from the fetch.
     * @return If true, then immediately release all native fetcher and response
     *     resources.  If false, then resources will be retained until garbage
     *     collected or manually released.  Note that the garbage collector is
     *     unaware of how many resources the native objects hold --- the
     *     response body might be multiple megabytes.
     */
    @CalledByNative
    boolean onBackgroundComplete(Fetcher fetcher, Response response);
}

