// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

public interface Response {
    /**
     * Get the response body from the fetch.
     *
     * @return null if no response body; otherwise the response body.
     */
    public byte[] getBody();

    /**
     * Get the length of the response body returned by the fetch.
     * This allows access to the length without bringing the entire
     * response body from native into Java.
     */
    public int getBodyLength();

    /**
     * Get the URL used for the intial fetch, prior to redirects.
     * This is the URL interpreted by the network stack --- it may be
     * changed from the URL supplied to Cnet into a canonical format.
     * @return the URL if it is not empty; otherwise null.
     */
    public String getOriginalUrl();

    /**
     * Get the URL used for the final redirect.
     * @return the URL if it is not empty; otherwise null.
     */
    public String getFinalUrl();

    /**
     * Get the HTTP response code from the fetch.
     * @return -1 if there was no HTTP response code (e.g., if the fetch
     *            was cancelled); otherwise the HTTP response code.
     */
    public int getHttpResponseCode();

    /**
     * Get all values of an HTTP response header.
     * This performs a case-insensitive comparison of the header name.
     * @return null if there was no matching header, otherwise an array of
     *         all values for the header.
     */
    public String[] getResponseHeader(String headerName);

    public String getResponseFirstHeader(String headerName);

    /**
     * Release the native resources used by this object.
     * This ensures immediate recovery of native memory, instead of waiting
     * for the Java garbage collector that is unaware of the amount of
     * memory consumed by the native representation.
     */
    public void release();
}

