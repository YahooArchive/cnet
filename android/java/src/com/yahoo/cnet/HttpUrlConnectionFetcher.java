// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;

public class HttpUrlConnectionFetcher implements Fetcher {
    public static final String LOG_TAG = HttpUrlConnectionFetcher.class.getName();
    public static final int MAX_RESPONSE_PREALLOC_BYTES = 5*1024*1024;

    private final HttpUrlConnectionPool mPool;
    private final String mUrl;
    private final String mMethod;
    private final ResponseCompletion mResponseCompletion;

    private boolean mIsCancelled; // Need to be locked.

    public HttpUrlConnectionFetcher(HttpUrlConnectionPool pool, String url,
                                    String method,
                                    ResponseCompletion responseCompletion) {
        mPool = pool;
        mUrl = url;
        mMethod = method;
        mResponseCompletion = responseCompletion;
    }

    @Override
    public void start() {
        mPool.getThreadPool().execute(new Runnable() {
            @Override
            public void run() {
                URL originalUrl = null;
                URL finalUrl = null;
                int httpResponseCode = -1;
                Map<String, List<String>> responseHeaders = null;
                byte[] responseBody  = null;

                try {
                    originalUrl = new URL(mUrl);
                }  catch (MalformedURLException e) {
                    Log.e(LOG_TAG, "Error in URL format.", e);
                }
                finalUrl = originalUrl;
                if (!isCancelled() && (originalUrl != null)) {
                    HttpURLConnection conn = null;
                    try {
                        URLConnection urlConn = originalUrl.openConnection();
                        if (urlConn instanceof HttpURLConnection) {
                            conn = (HttpURLConnection)urlConn;
                            conn.setUseCaches(false); // TODO
                            if (mPool.getUserAgent() != null) {
                                conn.setRequestProperty("User-Agent", mPool.getUserAgent());
                            }
                        } else {
                            Log.w(LOG_TAG, "Not an HTTP connection: " + originalUrl.toString());
                        }
                    } catch (IOException e) {
                        Log.e(LOG_TAG, "Error opening url.", e);
                    }
                    if (!isCancelled() && (conn != null)) {
                        InputStream in = null;
                        ByteArrayOutputStream baos = null;
                        int contentLength = 0;
                        try {
                            httpResponseCode = conn.getResponseCode();
                            responseHeaders = conn.getHeaderFields();
                            contentLength = conn.getContentLength();
                            finalUrl = conn.getURL();
                            in = conn.getInputStream();
                        } catch (IOException e) {
                            Log.e(LOG_TAG, "Error getting response.", e);
                        }
                        if (!isCancelled() && (in != null)) {
                            int preallocLength = (contentLength > MAX_RESPONSE_PREALLOC_BYTES) ?
                                    MAX_RESPONSE_PREALLOC_BYTES : (contentLength > 0 ? contentLength : 0);
                            try {
                                baos = (preallocLength > 0) ?
                                        new ByteArrayOutputStream(preallocLength) :
                                        new ByteArrayOutputStream();
                                byte[] buf = new byte[1024];
                                int readBytes = 0;
                                while (!isCancelled() && ((readBytes = in.read(buf)) > -1)) {
                                    baos.write(buf, 0, readBytes);
                                }
                            } catch (IOException e) {
                                Log.e(LOG_TAG, "Unable to read response.", e);
                                baos = null;
                            } finally {
                                try {
                                    in.close();
                                } catch (IOException e) {
                                }
                            }
                        }
                        if (!isCancelled() && (baos != null)) {
                            responseBody = baos.toByteArray();
                        }
                    }
                }

                if (mResponseCompletion != null) {
                    HttpUrlConnectionResponse response = new HttpUrlConnectionResponse(
                            originalUrl, finalUrl, responseHeaders, httpResponseCode, responseBody);
                    boolean release_now = mResponseCompletion.onBackgroundComplete(
                            HttpUrlConnectionFetcher.this, response);
                    if (release_now) {
                        response.release();
                    }
                }
            }
        });
    }

    private boolean isCancelled() {
        synchronized (this) {
            return mIsCancelled;
        }
    }

    @Override
    public void cancel() {
        synchronized (this) {
            mIsCancelled = true;
        }
    }

    @Override
    public void release() {  }

    @Override
    public void setCacheBehavior(int behavior) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setOauthCredentials(String appKey, String appSecret,
                                    String token, String tokenSecret) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUrlParamsEncoding(int encoding) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUrlParam(String key, String value) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void addUrlParams(Map<String, String> params) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUrlParamFile(String key, String filename,
                                String contentType, String path,
                                long rangeOffset, long rangeLength) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUploadBody(String contentType, String body) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUploadBody(String contentType, byte[] body) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setUploadFilePath(String contentType, String path,
                                  long rangeOffset, long rangeLength) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void setHeader(String key, String value) {
        throw new UnsupportedOperationException("unimplemented");
    }

    @Override
    public void addHeaders(Map<String, String> headers) {
        throw new UnsupportedOperationException("unimplemented");
    }
}
