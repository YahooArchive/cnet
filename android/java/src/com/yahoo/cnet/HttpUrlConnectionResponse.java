// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import android.graphics.Bitmap;

import java.net.URL;
import java.util.List;
import java.util.Map;

public class HttpUrlConnectionResponse implements Response {
    private final String mOriginalUrl;
    private final String mFinalUrl;
    private final Map<String, List<String>> mResponseHeaders;
    private final int mHttpResponseCode;
    private final byte[] mResponseBody;

    public HttpUrlConnectionResponse(URL originalUrl, URL finalUrl,
                                     Map<String, List<String>> responseHeaders,
                                     int httpResponseCode,
                                     byte[] responseBody) {
        mOriginalUrl = originalUrl.toString();
        mFinalUrl = finalUrl.toString();
        mResponseHeaders = responseHeaders;
        mHttpResponseCode = httpResponseCode;
        mResponseBody = responseBody;
    }

    @Override
    public byte[] getBody() {
        return mResponseBody;
    }

    @Override
    public boolean getBodyAsBitmap(Bitmap recycledBitmap,
            int maxWidth, int maxHeight, int scaleType) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public int getBodyLength() {
        if (mResponseBody == null) {
            return 0;
        } else {
            return mResponseBody.length;
        }
    }

    @Override
    public String getOriginalUrl() { return mOriginalUrl; }

    @Override
    public String getFinalUrl() { return mFinalUrl; }

    @Override
    public int getHttpResponseCode() { return mHttpResponseCode; }

    @Override
    public String[] getResponseHeader(String headerName) {
        if (mResponseHeaders == null) {
            return null;
        } else {
            List<String> list = mResponseHeaders.get(headerName);
            if ((list == null) || (list.size() == 0)) {
                return null;
            } else if (list.size() == 1) {
                String[] array = new String[1];
                array[0] = list.get(0);
                return array;
            } else {
                String[] array = new String[list.size()];
                list.toArray(array);
                return array;
            }
        }
    }

    @Override
    public String getResponseFirstHeader(String headerName) {
        if (mResponseHeaders == null) {
            return null;
        } else {
            List<String> list = mResponseHeaders.get(headerName);
            if ((list == null) || (list.size() == 0)) {
                return null;
            } else {
                return list.get(0);
            }
        }
    }

    @Override
    public void release() { }
}
