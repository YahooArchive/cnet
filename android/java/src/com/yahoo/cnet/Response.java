// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * Captures the response state of a fetch.
 */
@JNINamespace("cnet::android")
public class Response {
    private byte[] mBody;

    /**
     * Get the response body from the fetch.
     * @return null if no response body; otherwise the response body.
     */
    public synchronized byte[] getBody() {
        if ((mBody == null) && (mNativeResponseAdapter != 0)) {
            mBody = nativeGetBody(mNativeResponseAdapter);
        }
        return mBody;
    }

    /**
     * Get the length of the response body returned by the fetch.
     * This allows access to the length without bringing the entire
     * response body from native into Java.
     */
    public synchronized int getBodyLength() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetBodyLength(mNativeResponseAdapter);
        } else {
            return 0;
        }
    }

    /**
     * Get a native pointer to the response body.
     * This pointer can be passed to another native module,
     * allowing sharing of the response body without performing a
     * Java memory allocation.
     */
    public synchronized long getBodyRawPointer() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetBodyRawPointer(mNativeResponseAdapter);
        } else {
            return 0;
        }
    }

    /**
     * Get the URL used for the intial fetch, prior to redirects.
     * This is the URL interpreted by the network stack --- it may be
     * changed from the URL supplied to Cnet into a canonical format.
     * @return the URL if it is not empty; otherwise null.
     */
    public synchronized String getOriginalUrl() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetOriginalUrl(mNativeResponseAdapter);
        } else {
            return null;
        }
    }

    /**
     * Get the URL used for the final redirect.
     * @return the URL if it is not empty; otherwise null.
     */
    public synchronized String getFinalUrl() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetFinalUrl(mNativeResponseAdapter);
        } else {
            return null;
        }
    }

    /**
     * Get the HTTP response code from the fetch.
     * @return -1 if there was no HTTP response code (e.g., if the fetch
     *            was cancelled); otherwise the HTTP response code.
     */
    public synchronized int getHttpResponseCode() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetHttpResponseCode(mNativeResponseAdapter);
        } else {
            return -1;
        }
    }

    /**
     * Get the Chromium network error for the response.
     * The values are defined in {@link org.chromium.net.NetError}.
     * For more details, see:
     * <a href="http://src.chromium.org/svn/trunk/src/net/base/net_error_list.h">net_error_list.h</a>.
     *
     * Ranges:
     *    9- 99: System errors.
     *  100-199: Connection errors.
     *  200-299: Certificate errors.
     *  300-399: HTTP errors.
     *  400-499: Cache errors.
     *  500-599: Security and crypto errors.
     *  600-699: FTP errors.
     *  700-799: Certificate-manager errors.
     *  800-899: DNS-resolver errors.
     */
    public synchronized int getNetError() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetNetError(mNativeResponseAdapter);
        } else {
            return 0;
        }
    }

    /**
     * Get all values of an HTTP response header.
     * This performs a case-insensitive comparison of the header name.
     * @return null if there was no matching header, otherwise an array of
     *         all values for the header.
     */
    public synchronized String[] getResponseHeader(String headerName) {
        if (mNativeResponseAdapter != 0) {
            return nativeGetResponseHeader(mNativeResponseAdapter, headerName);
        } else {
            return null;
        }
    }

    public synchronized String getResponseFirstHeader(String headerName) {
        if (mNativeResponseAdapter != 0) {
            String[] headers = nativeGetResponseHeader(mNativeResponseAdapter, headerName);
            if ((headers != null) && (headers.length > 0)) {
                return headers[0];
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    /**
     * Release the native resources used by this object.
     * This ensures immediate recovery of native memory, instead of waiting
     * for the Java garbage collector that is unaware of the amount of
     * memory consumed by the native representation.
     */
    @CalledByNative
    public synchronized void release() {
        if (mNativeResponseAdapter != 0) {
            nativeReleaseResponseAdapter(mNativeResponseAdapter);
            mNativeResponseAdapter = 0;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        release();
        super.finalize();
    }

    @CalledByNative
    private static Response create(long nativeResponse) {
        return new Response(nativeResponse);
    }

    private Response(long nativeResponseAdapter) {
        mNativeResponseAdapter = nativeResponseAdapter;
    }

    private long mNativeResponseAdapter;

    private native byte[] nativeGetBody(long nativeResponseAdapter);
    private native int nativeGetBodyLength(long nativeResponseAdapter);
    private native long nativeGetBodyRawPointer(long nativeResponseAdapter);
    private native String nativeGetOriginalUrl(long nativeResponseAdapter);
    private native String nativeGetFinalUrl(long nativeResponseAdapter);
    private native int nativeGetHttpResponseCode(long nativeResponseAdapter);
    private native int nativeGetNetError(long nativeResponseAdapter);
    private native void nativeReleaseResponseAdapter(
            long nativeResponseAdapter);
    private native String[] nativeGetResponseHeader(long nativeResponseAdapter,
            String headerName);
}

