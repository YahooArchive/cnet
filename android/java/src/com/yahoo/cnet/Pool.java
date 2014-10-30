// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package com.yahoo.cnet;

public interface Pool {
    public void release();

    /**
     * Create a fetcher to perform a URL request.
     * The fetcher is prepared to run, but isn't active yet.  You must
     * separately start the fetcher to activate it.
     * @param url The URL of the request.
     * @param method The HTTP method, such as "GET", "POST", "PUT", etc.
     * @param responseCompletion The asynchronous completion, which will
     *        run on a background thread.
     */
    public Fetcher createFetcher(String url, String method,
            ResponseCompletion responseCompletion);

    /**
     * Override the system proxy settings.
     * Manually set an override proxy setting, such as "http://localhost:8888",
     * for debugging.  This will override the system's proxy settings.
     * To clear the override, and to revert to the system's proxy settings,
     * use null or an empty string.
     */
    public void setProxyRules(String rules);

    /**
     * Enable or disable self-signed certificate authorities.
     * For testing with a proxy, this can enable the SSL layer to accept
     * self-signed certificate authorities, and thus allow man-in-the-middle
     * attacks.  This will only work on debug builds --- release builds
     * ignore this setting.
     */
    public void setTrustAllCertAuthorities(boolean value);
    /**
     * Query for whether the pool accepts self-signed certificates.
     * Returns true if it accepts self-signed certificates.
     */
    public boolean getTrustAllCertAuthorities();

    /**
     * Enable or disable SSL false start.
     */
    public void setEnableSslFalseStart(boolean value);
    /**
     * Query whether SSL FalseStart is enabled.
     * Returns true if it is enabled.
     */
    public boolean getEnableSslFalseStart();

    /**
     * Preconnect to a remote host for use by future Fetchers.
     * It will preconnect multiple streams.
     */
    public void preconnect(String url, int numStreams);

    /**
     * QUIC protocol: add a host hint.
     * Specify that an HTTP request to a particular host and port
     * should instead use QUIC at the alternate port (alternatePort).
     */
    public void addQuicHint(String host, int port, int alternatePort);
}

