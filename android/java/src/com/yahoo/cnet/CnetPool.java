// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

@JNINamespace("cnet::android")
public class CnetPool implements Pool {
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

    public CnetPool(Config config) {
        mNativePoolAdapter = nativeCreatePoolAdapter(config.userAgent,
                config.enableSpdy, config.enableQuic,
                config.enableSslFalseStart, config.cachePath,
                config.cacheMaxBytes, config.trustAllCertAuthorities,
                config.disableSystemProxy, config.logLevel);
    }

    @Override
    public synchronized void release() {
        if (mNativePoolAdapter != 0) {
            nativeReleasePoolAdapter(mNativePoolAdapter);
            mNativePoolAdapter = 0;
        }
    }

    @Override
    public synchronized Fetcher createFetcher(String url, String method,
            ResponseCompletion responseCompletion) {
        if (mNativePoolAdapter != 0) {
            CnetFetcher fetcher = new CnetFetcher(mNativePoolAdapter, url,
                    method, responseCompletion);
            return fetcher;
        } else {
            return null;
        }
    }

    @Override
    public synchronized void setProxyRules(String rules) {
        if (mNativePoolAdapter != 0) {
            nativeSetProxyRules(mNativePoolAdapter, rules);
        }
    }

    @Override
    public synchronized void setTrustAllCertAuthorities(boolean value) {
        if (mNativePoolAdapter != 0) {
            nativeSetTrustAllCertAuthorities(mNativePoolAdapter, value);
        }
    }

    @Override
    public synchronized boolean getTrustAllCertAuthorities() {
        if (mNativePoolAdapter != 0) {
            return nativeGetTrustAllCertAuthorities(mNativePoolAdapter);
        } else {
            return false;
        }
    }

    @Override
    public synchronized void setEnableSslFalseStart(boolean value) {
        if (mNativePoolAdapter != 0) {
            nativeSetEnableSslFalseStart(mNativePoolAdapter, value);
        }
    }

    @Override
    public synchronized boolean getEnableSslFalseStart() {
        if (mNativePoolAdapter != 0) {
            return nativeGetEnableSslFalseStart(mNativePoolAdapter);
        } else {
            return false;
        }
    }

    @Override
    public synchronized void addQuicHint(String host, int port,
            int alternatePort) {
        if (mNativePoolAdapter != 0) {
            nativeAddQuicHint(mNativePoolAdapter, host, port, alternatePort);
        }
    }

    @Override
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

    private native void nativeAddQuicHint(long nativePoolAdapter, String host,
            int port, int alternatePort);

    private native void nativePreconnect(long nativePoolAdapter, String url,
            int numStreams);
}

