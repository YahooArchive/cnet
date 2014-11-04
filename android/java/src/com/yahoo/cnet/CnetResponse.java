// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import android.graphics.Bitmap;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * Captures the response state of a fetch.
 */
@JNINamespace("cnet::android")
public class CnetResponse implements Response {
    private byte[] mBody;

    @Override
    public synchronized byte[] getBody() {
        if ((mBody == null) && (mNativeResponseAdapter != 0)) {
            mBody = nativeGetBody(mNativeResponseAdapter);
        }
        return mBody;
    }

    @Override
    public synchronized boolean getBodyAsBitmap(Bitmap recycledBitmap,
            int maxWidth, int maxHeight, int scaleType) {
        if (mNativeResponseAdapter != 0) {
            if (nativeHasNativeBitmap()) {
                return nativeGetBodyAsBitmap(mNativeResponseAdapter,
                        recycledBitmap, maxWidth, maxHeight, scaleType);
            } else if (mBody == null) {
                mBody = nativeGetBody(mNativeResponseAdapter);
            }
        }

        if (mBody != null) {
            throw new UnsupportedOperationException("unimplemented");
        }
        return false;
    }

    @Override
    public synchronized int getBodyLength() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetBodyLength(mNativeResponseAdapter);
        } else {
            return 0;
        }
    }

    @Override
    public synchronized String getOriginalUrl() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetOriginalUrl(mNativeResponseAdapter);
        } else {
            return null;
        }
    }

    @Override
    public synchronized String getFinalUrl() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetFinalUrl(mNativeResponseAdapter);
        } else {
            return null;
        }
    }

    @Override
    public synchronized int getHttpResponseCode() {
        if (mNativeResponseAdapter != 0) {
            return nativeGetHttpResponseCode(mNativeResponseAdapter);
        } else {
            return -1;
        }
    }

    @Override
    public synchronized String[] getResponseHeader(String headerName) {
        if (mNativeResponseAdapter != 0) {
            return nativeGetResponseHeader(mNativeResponseAdapter, headerName);
        } else {
            return null;
        }
    }

    @Override
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

    public synchronized boolean wasCached() {
        if (mNativeResponseAdapter != 0) {
            return nativeWasCached(mNativeResponseAdapter);
        } else {
            return false;
        }
    }

    public synchronized boolean wasFetchedViaProxy() {
        if (mNativeResponseAdapter != 0) {
            return nativeWasFetchedViaProxy(mNativeResponseAdapter);
        } else {
            return false;
        }
    }

    public synchronized boolean wasFetchedViaSpdy() {
        if (mNativeResponseAdapter != 0) {
            return nativeWasFetchedViaSpdy(mNativeResponseAdapter);
        } else {
            return false;
        }
    }

    public synchronized boolean wasFetchedViaQuic() {
        if (mNativeResponseAdapter != 0) {
            return nativeWasFetchedViaQuic(mNativeResponseAdapter);
        } else {
            return false;
        }
    }

    @Override
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
    private static CnetResponse create(long nativeResponse) {
        return new CnetResponse(nativeResponse);
    }

    private CnetResponse(long nativeResponseAdapter) {
        mNativeResponseAdapter = nativeResponseAdapter;
    }

    private long mNativeResponseAdapter;

    private native byte[] nativeGetBody(long nativeResponseAdapter);
    private native int nativeGetBodyLength(long nativeResponseAdapter);
    private native long nativeGetBodyRawPointer(long nativeResponseAdapter);
    private native boolean nativeHasNativeBitmap();
    private native boolean nativeGetBodyAsBitmap(long nativeResponseAdapter,
            Bitmap recycledBitmap, int maxWidth, int maxHeight, int scaleType);
    private native String nativeGetOriginalUrl(long nativeResponseAdapter);
    private native String nativeGetFinalUrl(long nativeResponseAdapter);
    private native int nativeGetHttpResponseCode(long nativeResponseAdapter);
    private native int nativeGetNetError(long nativeResponseAdapter);
    private native void nativeReleaseResponseAdapter(
            long nativeResponseAdapter);
    private native String[] nativeGetResponseHeader(long nativeResponseAdapter,
            String headerName);
    private native boolean nativeWasCached(long nativeResponseAdapter);
    private native boolean nativeWasFetchedViaProxy(long nativeResponseAdapter);
    private native boolean nativeWasFetchedViaSpdy(long nativeResponseAdapter);
    private native boolean nativeWasFetchedViaQuic(long nativeResponseAdapter);
}

