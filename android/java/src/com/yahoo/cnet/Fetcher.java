// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

import java.util.Map;

public interface Fetcher {
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
     * Start the fetch.
     * You may no longer adjust the fetch properties once it is started.
     */
    public void start();

    /**
     * Cancel the fetch.
     * This tries to cancel the fetch.  The fetch may successfully complete
     * before the cancellation attempt proceeds. The fetch completion will
     * run even when the fetch is cancelled.
     */
    public void cancel();

    /**
     * Release the native resources used by this object.
     * This ensures immediate recovery of native memory, instead of waiting
     * for the Java garbage collector that is unaware of the amount of
     * memory consumed by the native representation.  The fetch will
     * continue to execute, and will invoke its completion, even if you
     * have released these resources.
     */
    public void release();

    public void setCacheBehavior(int behavior);

    /**
     * Set the OAuth v1 credentials.
     * When these credentials are set, the request will be signed
     * according to OAuth v1.
     */
    public void setOauthCredentials(String appKey, String appSecret, String token, String tokenSecret);

    public void setUrlParamsEncoding(int encoding);
    public void setUrlParam(String key, String value);
    public void addUrlParams(Map<String, String> params);
    public void setUrlParamFile(String key, String filename, String contentType, String path,
                                long rangeOffset, long rangeLength);
    public void setUploadBody(String contentType, String body);
    public void setUploadBody(String contentType, byte[] body);
    public void setUploadFilePath(String contentType, String path, long rangeOffset, long rangeLength);

    public void setHeader(String key, String value);
    public void addHeaders(Map<String, String> headers);
}

