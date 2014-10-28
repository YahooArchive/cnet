// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.util.Map;

@JNINamespace("cnet::android")
public class CnetFetcher implements Fetcher {
    /**
     * Create a fetcher to perform a URL request.
     * The fetcher is prepared to run, but isn't active yet.  You must
     * separately start the fetch to activate it.
     * @param pool The Cnet pool to provide resources for the request.
     * @param url The URL of the request.
     * @param method The HTTP method, such as "GET", "POST", "PUT", etc.
     * @param responseCompletion The asynchronous completion, which will
     *        run on a background thread.
     */
    public CnetFetcher(Pool pool, String url, String method,
            ResponseCompletion responseCompletion) {
        mNativeFetcherAdapter = nativeCreateFetcherAdapter(pool, url, method,
                responseCompletion);
    }

    @Override
    @CalledByNative
    public synchronized void release() {
        if (mNativeFetcherAdapter != 0) {
            nativeReleaseFetcherAdapter(mNativeFetcherAdapter);
            mNativeFetcherAdapter = 0;
        }
    }

    @Override
    public synchronized void start() {
        if (mNativeFetcherAdapter != 0) {
            nativeStart(mNativeFetcherAdapter);
        }
    }

    @Override
    public synchronized void cancel() {
        if (mNativeFetcherAdapter != 0) {
            nativeCancel(mNativeFetcherAdapter);
        }
    }

    @Override
    public synchronized void setCacheBehavior(int behavior) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetCacheBehavior(mNativeFetcherAdapter, behavior);
        }
    }

    @Override
    public synchronized void setOauthCredentials(String appKey,
            String appSecret, String token, String tokenSecret) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetOauthCredentials(mNativeFetcherAdapter, appKey, appSecret,
                    token, tokenSecret);
        }
    }

    @Override
    public synchronized void setUrlParamsEncoding(int encoding) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParamsEncoding(mNativeFetcherAdapter, encoding);
        }
    }

    @Override
    public synchronized void setUrlParam(String key, String value) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParam(mNativeFetcherAdapter, key, value);
        }
    }

    @Override
    public synchronized void addUrlParams(Map<String, String> params) {
        if (mNativeFetcherAdapter != 0) {
            for (Map.Entry<String, String> entry : params.entrySet()) {
                nativeSetUrlParam(mNativeFetcherAdapter, entry.getKey(),
                        entry.getValue());
            }
        }
    }

    @Override
    public synchronized void setUrlParamFile(String key, String filename,
            String contentType, String path,
            long rangeOffset, long rangeLength) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParamFile(mNativeFetcherAdapter, key, filename,
                    contentType, path, rangeOffset, rangeLength);
        }
    }

    @Override
    public synchronized void setUploadBody(String contentType, String body) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadStringBody(mNativeFetcherAdapter, contentType, body);
        }
    }

    @Override
    public synchronized void setUploadBody(String contentType, byte[] body) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadByteBody(mNativeFetcherAdapter, contentType, body);
        }
    }

    @Override
    public synchronized void setUploadFilePath(String contentType, String path,
            long rangeOffset, long rangeLength) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadFilePath(mNativeFetcherAdapter, contentType, path,
                    rangeOffset, rangeLength);
        }
    }

    @Override
    public synchronized void setHeader(String key, String value) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetHeader(mNativeFetcherAdapter, key, value);
        }
    }

    @Override
    public synchronized void addHeaders(Map<String, String> headers) {
        if (mNativeFetcherAdapter != 0) {
            for (Map.Entry<String, String> entry : headers.entrySet()) {
                nativeSetHeader(mNativeFetcherAdapter, entry.getKey(),
                        entry.getValue());
            }
        }
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    private long mNativeFetcherAdapter;

    private native long nativeCreateFetcherAdapter(Pool pool, String url,
            String method, ResponseCompletion responseCompletion);
    private native void nativeReleaseFetcherAdapter(long nativeFetcherAdapter);

    private native void nativeStart(long nativeFetcherAdapter);
    private native void nativeCancel(long nativeFetcherAdapter);

    private native void nativeSetCacheBehavior(long nativeFetcherAdapter,
            int behavior);

    private native void nativeSetOauthCredentials(long nativeFetcherAdapter,
            String appKey, String appSecret, String token, String tokenSecret);

    private native void nativeSetUrlParamsEncoding(long nativeFetcherAdapter,
            int encoding);
    private native void nativeSetUrlParam(long nativeFetcherAdapter, String key,
            String value);
    private native void nativeSetUrlParamFile(long nativeFetcherAdapter,
            String key, String filename, String contentType, String path,
            long rangeOffset, long rangeLength);

    private native void nativeSetUploadStringBody(long nativeFetcherAdapter,
            String contentType, String body);
    private native void nativeSetUploadByteBody(long nativeFetcherAdapter,
            String contentType, byte[] body);
    private native void nativeSetUploadFilePath(long nativeFetcherAdapter,
            String contentType, String path,
            long rangeOffset, long rangeLength);

    private native void nativeSetHeader(long nativeFetcherAdapter, String key,
            String value);
}

