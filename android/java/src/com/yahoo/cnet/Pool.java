// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

@JNINamespace("cnet::android")
public class Pool {
    static public class Config {
        public String userAgent;

        public boolean enableSpdy;
        public boolean enableQuic;

        public boolean enableSslFalseStart;
        public boolean trustAllCertAuthorities;
        public boolean disableSystemProxy;

        public String cachePath;
        public int cacheMaxBytes;

        public int logLevel;
    }

    public Pool(Config config) {
        mNativePoolAdapter = nativeCreatePoolAdapter(config.userAgent,
                config.enableSpdy, config.enableQuic,
                config.enableSslFalseStart, config.cachePath,
                config.cacheMaxBytes, config.trustAllCertAuthorities,
                config.disableSystemProxy, config.logLevel);
    }

    public synchronized void release() {
        if (mNativePoolAdapter != 0) {
            nativeReleasePoolAdapter(mNativePoolAdapter);
            mNativePoolAdapter = 0;
        }
    }

    /**
     * Override the system proxy settings.
     * Manually set an override proxy setting, such as "http://localhost:8888",
     * for debugging.  This will override the system's proxy settings.
     * To clear the override, and to revert to the system's proxy settings, 
     * set an empty string for the rules.
     */
    public synchronized void setProxyRules(String rules) {
        if (mNativePoolAdapter != 0) {
            nativeSetProxyRules(mNativePoolAdapter, rules);
        }
    }

    /**
     * Enable or disable self-signed certificate authorities.
     * For testing with a proxy, this can enable the SSL layer to accept
     * self-signed certificate authorities, and thus allow man-in-the-middle
     * attacks.  This will only work on debug builds --- release builds
     * ignore this setting.
     */
    public synchronized void setTrustAllCertAuthorities(boolean value) {
        if (mNativePoolAdapter != 0) {
            nativeSetTrustAllCertAuthorities(mNativePoolAdapter, value);
        }
    }

    public synchronized boolean getTrustAllCertAuthorities() {
        if (mNativePoolAdapter != 0) {
            return nativeGetTrustAllCertAuthorities(mNativePoolAdapter);
        } else {
            return false;
        }
    }

    /**
     * Enable or disable SSL false start.
     */
    public synchronized void setEnableSslFalseStart(boolean value) {
        if (mNativePoolAdapter != 0) {
            nativeSetEnableSslFalseStart(mNativePoolAdapter, value);
        }
    }

    public synchronized boolean getEnableSslFalseStart() {
        if (mNativePoolAdapter != 0) {
            return nativeGetEnableSslFalseStart(mNativePoolAdapter);
        } else {
            return false;
        }
    }

    /**
     * Preconnect to a remote host for use by future Fetchers.
     * It will preconnect multiple streams.
     */
    public synchronized void preconnect(String url, int numStreams) {
        if (mNativePoolAdapter != 0) {
            nativePreconnect(mNativePoolAdapter, url, numStreams);
        }
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @CalledByNative
    private synchronized long getNativePoolAdapter() {
        return mNativePoolAdapter;
    }

    private long mNativePoolAdapter;

    private native long nativeCreatePoolAdapter(String userAgent,
            boolean enableSpdy, boolean enableQuic, boolean enableSslFalseStart,
            String cachePath, int cacheMaxBytes,
            boolean trustAllCertAuthorities, boolean disableSystemProxy,
            int logLevel);

    private native void nativeReleasePoolAdapter(long nativePoolAdapter);

    private native void nativeSetProxyRules(long nativePoolAdapter,
            String rules);

    private native void nativeSetTrustAllCertAuthorities(long nativePoolAdapter,
            boolean value);
    private native boolean nativeGetTrustAllCertAuthorities(
            long nativePoolAdapter);

    private native void nativeSetEnableSslFalseStart(long nativePoolAdapter,
            boolean value);
    private native boolean nativeGetEnableSslFalseStart(long nativePoolAdapter);

    private native void nativePreconnect(long nativePoolAdapter, String url,
            int numStreams);
}

