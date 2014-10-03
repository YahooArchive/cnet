// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

import java.util.Map;

@JNINamespace("cnet::android")
public class Fetcher {
    /**
     * Use protocol-appropriate caching.
     */
    public static final int CACHE_NORMAL = 0;
    /**
     * Like a browser reload, w/ an if-none-match/if-modified-since query.
     */
    public static final int CACHE_VALIDATE = 1;
    /**
     * Like a browser shift-reload, w/ a "pragma: no-cache" end-to-end fetch.
     */
    public static final int CACHE_BYPASS = 2;
    /**
     * Like browser back/forward; cached content is preferred over protocol-
     * specific cache validation.
     */
    public static final int CACHE_PREFER = 3;
    /**
     * Will fail if the file can't be retrieved from the cache.
     */
    public static final int CACHE_ONLY = 4;
    /**
     * If the request fails due to networking, then behave as if
     * CACHE_PREFER was specified.
     */
    public static final int CACHE_IF_OFFLINE = 5;
    /**
     * Will skip the local cache; it doesn't change the HTTP headers.
     */
    public static final int CACHE_DISABLE = 6;

    /**
     * Encode parameters as the query of the URL.  A file can be sent as the
     * body.
     */
    public static final int PARAMS_ENCODE_URL = 0;
    /**
     * Encode parameters as a multi-part form.  A file can be sent as one of
     * the multi-part parameters.
     */
    public static final int PARAMS_ENCODE_BODY_MULTIPART = 1;
    /**
     * Encode parameters with URL encoding, but send as the body.  No
     * file can be sent.
     */
    public static final int PARAMS_ENCODE_BODY_URL = 2;

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
    public Fetcher(Pool pool, String url, String method,
            ResponseCompletion responseCompletion) {
        mNativeFetcherAdapter = nativeCreateFetcherAdapter(pool, url, method,
                responseCompletion);
    }

    /**
     * Release the native resources used by this object.
     * This ensures immediate recovery of native memory, instead of waiting
     * for the Java garbage collector that is unaware of the amount of
     * memory consumed by the native representation.  The fetch will
     * continue to execute, and will invoke its completion, even if you
     * have released these resources.
     */
    @CalledByNative
    public synchronized void release() {
        if (mNativeFetcherAdapter != 0) {
            nativeReleaseFetcherAdapter(mNativeFetcherAdapter);
            mNativeFetcherAdapter = 0;
        }
    }

    /**
     * Start the fetch.
     * You may no longer adjust the fetch properties once it is started.
     */
    public synchronized void start() {
        if (mNativeFetcherAdapter != 0) {
            nativeStart(mNativeFetcherAdapter);
        }
    }

    /**
     * Cancel the fetch.
     * This tries to cancel the fetch.  The fetch may successfully complete
     * before the cancellation attempt proceeds. The fetch completion will
     * run even when the fetch is cancelled.
     */
    public synchronized void cancel() {
        if (mNativeFetcherAdapter != 0) {
            nativeCancel(mNativeFetcherAdapter);
        }
    }

    public synchronized void setCacheBehavior(int behavior) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetCacheBehavior(mNativeFetcherAdapter, behavior);
        }
    }

    /**
     * Set the OAuth v1 credentials.
     * When these credentials are set, the request will be signed
     * according to OAuth v1.
     */
    public synchronized void setOauthCredentials(String appKey,
            String appSecret, String token, String tokenSecret) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetOauthCredentials(mNativeFetcherAdapter, appKey, appSecret,
                    token, tokenSecret);
        }
    }

    public synchronized void setUrlParamsEncoding(int encoding) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParamsEncoding(mNativeFetcherAdapter, encoding);
        }
    }

    public synchronized void setUrlParam(String key, String value) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParam(mNativeFetcherAdapter, key, value);
        }
    }

    public synchronized void addUrlParams(Map<String, String> params) {
        if (mNativeFetcherAdapter != 0) {
            for (Map.Entry<String, String> entry : params.entrySet()) {
                nativeSetUrlParam(mNativeFetcherAdapter, entry.getKey(),
                        entry.getValue());
            }
        }
    }

    public synchronized void setUrlParamFile(String key, String filename,
            String contentType, String path,
            long rangeOffset, long rangeLength) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUrlParamFile(mNativeFetcherAdapter, key, filename,
                    contentType, path, rangeOffset, rangeLength);
        }
    }

    public synchronized void setUploadBody(String contentType, String body) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadStringBody(mNativeFetcherAdapter, contentType, body);
        }
    }

    public synchronized void setUploadBody(String contentType, byte[] body) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadByteBody(mNativeFetcherAdapter, contentType, body);
        }
    }

    public synchronized void setUploadFilePath(String contentType, String path,
            long rangeOffset, long rangeLength) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetUploadFilePath(mNativeFetcherAdapter, contentType, path,
                    rangeOffset, rangeLength);
        }
    }

    public synchronized void setHeader(String key, String value) {
        if (mNativeFetcherAdapter != 0) {
            nativeSetHeader(mNativeFetcherAdapter, key, value);
        }
    }

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

