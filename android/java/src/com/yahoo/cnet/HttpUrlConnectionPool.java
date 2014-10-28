// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class HttpUrlConnectionPool implements Pool {
    static public class Config {
        public String userAgent;
        public int threadCount;
    }

    private final BlockingQueue<Runnable> mRunQueue;
    private final ThreadPoolExecutor mThreadPool;
    private final String mUserAgent;

    public HttpUrlConnectionPool(final Config config) {
        int threadCount = 3;
        if (config != null) {
            if (config.threadCount > 0) {
                threadCount = config.threadCount;
            }
            mUserAgent = config.userAgent;
        } else {
            mUserAgent = null;
        }

        mRunQueue = new LinkedBlockingQueue<Runnable>();
        mThreadPool = new ThreadPoolExecutor(threadCount, threadCount, 10, TimeUnit.SECONDS, mRunQueue);
    }

    @Override
    public Fetcher createFetcher(String url, String method, ResponseCompletion responseCompletion) {
        return new HttpUrlConnectionFetcher(this, url, method, responseCompletion);
    }

    @Override
    public void setProxyRules(String rules) { }

    @Override
    public void setTrustAllCertAuthorities(boolean value) { }

    @Override
    public boolean getTrustAllCertAuthorities() { return false; }

    @Override
    public void setEnableSslFalseStart(boolean value) { }

    @Override
    public boolean getEnableSslFalseStart() { return false; }

    @Override
    public void addQuicHint(String host, int port, int alternatePort) { }

    @Override
    public void preconnect(String url, int numStreams) { }

    @Override
    public void release() { }

    public String getUserAgent() { return mUserAgent; }
    public ThreadPoolExecutor getThreadPool() { return mThreadPool; }
}

