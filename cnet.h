// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_H_
#define YAHOO_CNET_CNET_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#define CNET_EXPORT __declspec(dllexport)
#else
#define CNET_EXPORT __attribute__((visibility("default")))
#endif

// A CnetPool tracks resources shared between HTTP requests.
typedef void* CnetPool;

// A CnetFetcher performs an HTTP request.
typedef void* CnetFetcher;

// The HTTP response from a CnetFetcher;
typedef void* CnetResponse;

// A CnetMessageLopForUI represents the platform's UI message-dispatching loop.
typedef void* CnetMessageLoopForUi;


// Create a connection to the platform's UI message-dispatching loop.
// This must be called from the platform's UI loop.
CNET_EXPORT CnetMessageLoopForUi CnetMessageLoopForUiGet();

// Initialize the Cnet library.  On Android you must also invoke
// CnetJniInitializeAppContext(), as documented in cnet_jni.h.
CNET_EXPORT void CnetInitialize(int in_chromium);

// Cleanup the Cnet library's global resources.  Invoke this on the same
// thread that called CnetInitialize().
CNET_EXPORT void CnetCleanup();


typedef struct {
  // The user agent issued with each HTTP request.
  const char* user_agent;
  // Allow HTTP requests to use the SPDY protocol.
  int enable_spdy;
  // Allow HTTP requests to use the QUIC protocol.
  int enable_quic;
  // Enable faster SSL connections.
  int enable_ssl_false_start;
  // The file-system path to a directory dedicated as a file cache.
  // If NULL, the cache is disabled.
  const char* cache_path;
  // The maximum number of bytes to store in the cache.
  // If 0, the cache is disabled.
  unsigned cache_max_bytes;
  // Allow man-in-the-middle attacks, by trusting self-signed root certificate
  // authorities.  This can be enabled for debug builds only.  Useful for
  // Charles Proxy.
  int trust_all_cert_authorities;
  // Disable use of the system's proxy settings.  Thus the only
  // proxy settings that will be used are those set manually
  // via the Cnet API.
  int disable_system_proxy;
  // Include more data in the log if greater than 0.
  //   1: include more error conditions.
  //   2: include telemetry.
  int log_level;
} CnetPoolConfig;

CNET_EXPORT void CnetPoolDefaultConfigPrepare(CnetPoolConfig* config);

// Create a CnetPool.  It is returned with a retain count of 1.
//   ui_loop: the UI's message-dispatch loop, used for listening for changes
//       to the proxy configuration.  If NULL, the proxy changes may be
//       ignored.
//   config: the initial configuration.
CNET_EXPORT CnetPool CnetPoolCreate(CnetMessageLoopForUi ui_loop,
    CnetPoolConfig config);

// Increase the retain count on a CnetPool.
CNET_EXPORT CnetPool CnetPoolRetain(CnetPool pool);

// Decrease the retain count on a CnetPool.  When it reaches 0, it is
// deleted.
CNET_EXPORT void CnetPoolRelease(CnetPool pool);

// Override the system proxy settings, such as when using Charles Proxy.
// An example rule: http://10.73.218.92:8888
CNET_EXPORT void CnetPoolSetProxy(CnetPool pool, const char* rules);

// Enable or disable SSL False Start for all future network connections.
CNET_EXPORT void CnetPoolSetEnableSslFalseStart(CnetPool pool, int enabled);

// Trust or distrust self-signed root certificate authorities.  This
// manipulates debug builds only.  Useful for Charles Proxy.
CNET_EXPORT void CnetPoolSetTrustAllCertAuthorities(CnetPool pool, int enabled);

// Wait (and block the current thread) until a pool has no more outstanding
// requests.  This has no timeout, so use carefully.
CNET_EXPORT void CnetPoolDrain(CnetPool pool);

// For mass request cancellation by tag, register a fetcher with a tag.
// Multiple fetchers can be registered with the same tag.
CNET_EXPORT void CnetPoolTagFetcher(CnetPool pool, CnetFetcher fetcher, int tag);

// Cancel all fetchers associated with a tag.
CNET_EXPORT void CnetPoolCancelTag(CnetPool pool, int tag);


typedef enum {
  CNET_ENCODE_URL,
  CNET_ENCODE_BODY_MULTIPART,
  CNET_ENCODE_BODY_URL
} CnetUrlParamsEncoding;

typedef enum {
  CNET_CACHE_NORMAL,

  // Like a browser reload, w/ an if-none-match/if-modified-since query
  CNET_CACHE_VALIDATE,

  // Like a browser shift-reload, w/ a "pragma: no-cache" end-to-end fetch
  CNET_CACHE_BYPASS,

  // Like browser back/forward; cached content is preferred over protocol-
  // specific cache validation
  CNET_CACHE_PREFER,

  // Will fail if the file can't be retrieved from the cache
  CNET_CACHE_ONLY,

  // If the request fails due to networking, then behave as if
  // CACHE_PREFER was specified
  CNET_CACHE_IF_OFFLINE,

  // Will skip the local cache; it doesn't change the HTTP headers.
  CNET_CACHE_DISABLE,
} CnetCacheBehavior;

typedef struct CnetLoadTiming {
  // Start time, as seconds since the Unix Epoch
  double start_s;

  // Milliseconds spent in queues.
  uint32_t queued_ms;
  // Milliseconds spent resolving DNS.
  uint32_t dns_ms;
  // Milliseconds spent connecting.
  uint32_t connect_ms;
  // Milliseconds spent negotiating SSL.
  uint32_t ssl_ms;
  // Milliseconds spent preparing a proxy.
  uint32_t proxy_resolve_ms;
  // Milliseconds spent sending data.
  uint32_t send_ms;
  // Milliseconds spent receiving headers.
  uint32_t headers_receive_ms;
  // Milliseconds spent receiving data.
  uint32_t data_receive_ms;
  // Total milliseconds consumed by the request.
  uint32_t total_ms;

  // Was the result from the cache?
  int from_cache;
  // Total received bytes.
  int64_t total_recv_bytes;
  // Total sent bytes.
  int64_t total_send_bytes;

  // Was the socket reused?
  int socket_reused;
  // A logical socket ID, for tracking reused sockets.
  uint32_t socket_log_id;
} CnetLoadTiming;

// The completion callback for a request.  It is invoked on a background thread.
//   param: the parameter for use by the callback.
typedef void (*CnetFetcherCompletion)(CnetFetcher fetcher,
    CnetResponse response, void* param);

// The download/upload-progress callback for a request.  It is invoked on a
// background thread.
//   param: the parameter for use by the callback.
//   current: the number of bytes transferred so far.
//   total: the expected number of bytes to transfer; it is -1 if unknown.
typedef void (*CnetFetcherProgressCallback)(CnetFetcher fetcher, void* param,
    int64_t current, int64_t total);

// Create an HTTP request: a CnetFetcher.  It is returned with a retain
// count of 1.  It returns NULL in case of failure creating the request.
// This doesn't start the request.
//   callback_param: the parameter for use by the callbacks.
//   completion, download, upload: callbacks that execute on a background
//   thread.
CNET_EXPORT CnetFetcher CnetFetcherCreate(CnetPool pool, const char* url,
    const char* method, void* callback_param,
    CnetFetcherCompletion completion, CnetFetcherProgressCallback download,
    CnetFetcherProgressCallback upload);

// Increase the retain count on a fetcher.
CNET_EXPORT CnetFetcher CnetFetcherRetain(CnetFetcher fetcher);

// Decrease the retain count on a fetcher.  When it reaches 0, it is deleted.
CNET_EXPORT void CnetFetcherRelease(CnetFetcher fetcher);

// Start the HTTP request.
CNET_EXPORT void CnetFetcherStart(CnetFetcher fetcher);

// Cancel an active HTTP request.  The completion callback will execute.
CNET_EXPORT void CnetFetcherCancel(CnetFetcher fetcher);

// Retrieve the opaque user data from the Fetcher.
CNET_EXPORT void* CnetFetcherGetCallbackParam(CnetFetcher fetcher);

// Set a minimum transfer speed.  If the transfer speed falls below
// this threshold for more the duration, then it cancels with a
// timeout error.
//   min_speed_bytes_sec: the minimium speed in bytes per second.
//   duration_secs: the duration of the minimum speed in seconds.
CNET_EXPORT void CnetFetcherSetMinSpeed(CnetFetcher fetcher,
  double min_speed_bytes_sec, double duration_secs);

// Adjust the cache behavior of this HTTP request.  By default the
// request obey's the protocol's defined caching behavior.
CNET_EXPORT void CnetFetcherSetCacheBehavior(CnetFetcher fetcher,
    CnetCacheBehavior behavior);

// Control whether the request stops when encountering a redirect.
//   stop_on_redirect: if non-zero, than stop the request rather than
//       follow a redirect.  If zero, then follow redirects.
CNET_EXPORT void CnetFetcherSetStopOnRedirect(CnetFetcher fetcher,
    int stop_on_redirect);

// Add a request header.
CNET_EXPORT void CnetFetcherSetHeader(CnetFetcher fetcher,
    const char* key, const char* value);
CNET_EXPORT void CnetFetcherSetHeaderInt(CnetFetcher fetcher,
    const char* key, int value);
CNET_EXPORT void CnetFetcherSetHeaderDouble(CnetFetcher fetcher,
    const char* key, double value);

// When submitting parameters with the request, set the encoding type for
// the parameters.  If this is a body encoding, and a raw post body is already
// set, then the raw post body will be cleared.
CNET_EXPORT void CnetFetcherSetUrlParamsEncoding(CnetFetcher fetcher,
    CnetUrlParamsEncoding encoding);

// Set a request parameter.
CNET_EXPORT void CnetFetcherSetUrlParam(CnetFetcher fetcher,
    const char* key, const char* value);
CNET_EXPORT void CnetFetcherSetUrlParamInt(CnetFetcher fetcher,
    const char* key, int value);
CNET_EXPORT void CnetFetcherSetUrlParamDouble(CnetFetcher fetcher,
    const char* key, double value);

// Upload a file with a multi-part form encoding (setting the value of a
// a variable to the contents of the file).  If another encoding was set,
// this will force the encoding to a multi-part encoding.
//   key: the name of the variable in the multi-part form that gets assigned
//       the file's contents.
//   filename: extra information encoded with the key/value pair.
//   content_type: the MIME type for the uploaded file.
//   file_path: the path to the file; it will be read from a background thread.
//   range_offset: the starting byte in the file.
//   range_length: the size of the region to send from the file; use
//       the max value for uint64_t to specify the whole file.
CNET_EXPORT void CnetFetcherSetUrlParamFile(CnetFetcher fetcher,
    const char* key, const char* filename, const char* content_type,
    const char* file_path, uint64_t range_offset, uint64_t range_length);

// Set the request's OAuth v1 credentials.  If set, then the request
// will be signed according to OAuth v1.
CNET_EXPORT void CnetFetcherSetOauthCredentials(CnetFetcher fetcher,
    const char* app_key, const char* app_secret, const char* token,
    const char* token_secret);

// Set the request's post body and content type.  If url params are
// also set, then this will force those to be encoded in the URL.
CNET_EXPORT void CnetFetcherSetUploadBody(CnetFetcher fetcher,
    const char* content_type, const char* content);

// Use the contents of a file as the requests post body.
//   range_offset: the starting byte in the file.
//   range_length: the size of the region to send from the file; use the
//       max value for uint64_t to specify the whole file.
CNET_EXPORT void CnetFetcherSetUploadFile(CnetFetcher fetcher,
    const char* content_type, const char* path, uint64_t range_offset,
    uint64_t range_length);

// Save the response content to a file, and do not buffer in memory.
// If the path is NULL, then it will instead buffer the response in memory.
CNET_EXPORT void CnetFetcherSetOutputFile(CnetFetcher fetcher,
    const char* path);

// Get the fetcher's pool.
CNET_EXPORT CnetPool CnetFetcherPool(CnetFetcher fetcher);

// Get the URL used to initialize the fetcher.  The actual URL sent to the
// server will be canonical, and will include query parameters.
CNET_EXPORT const char* CnetFetcherInitialUrl(CnetFetcher fetcher);


// Increment the retain count on a response.
CNET_EXPORT CnetResponse CnetResponseRetain(CnetResponse response);

// Decrease the retain count on a response.  When it reaches 0, it is deleted.
CNET_EXPORT void CnetResponseRelease(CnetResponse response);

// Get the URL used to intialize the fetcher.  This may not be the URL
// sent to the server; the sent URL will be canonical, and may have
// a query.
CNET_EXPORT const char* CnetResponseInitialUrl(CnetResponse response);
// Get the URL used for the initial fetch, prior to redirects.
// This is the URL interpreted by the network stack --- it may be
// changed from the URL supplied to Cnet into a canonical format.
// Returns NULL if the URL is undefined.
CNET_EXPORT const char* CnetResponseCanonicalUrl(CnetResponse response);
// Get the final URL for the request, after redirects.  Returns NULL
// if the URL is undefined.
CNET_EXPORT const char* CnetResponseFinalUrl(CnetResponse response);
// Get the timing (telemetry) of the request.
CNET_EXPORT const CnetLoadTiming* CnetResponseTiming(CnetResponse response);
// Get a pointer to the response body of the request.
// Returns NULL if there was no response.
CNET_EXPORT const char* CnetResponseBody(CnetResponse response);
// Get the size of the response body.
CNET_EXPORT int CnetResponseLength(CnetResponse response);
// Returns true if the request succeeded.
CNET_EXPORT int CnetResponseSucceeded(CnetResponse response);
// Returns true if the request failed.
CNET_EXPORT int CnetResponseFailed(CnetResponse response);
// Returns true if the request was cancelled.
CNET_EXPORT int CnetResponseCancelled(CnetResponse response);
// Get the HTTP result code of the request, if it completed.
// It returns -1 if the request returned no valid response code
// (e.g., if it was cancelled).
CNET_EXPORT int CnetResponseHttpCode(CnetResponse response);
// Get the first header value of a header.  Returns NULL if there is no
// matching header.  You must free the memory when finished with it.
CNET_EXPORT char* CnetResponseFirstHeaderCopy(CnetResponse response,
    const char* header_name);

#ifdef __cplusplus
};
#endif

#endif  // YAHOO_CNET_CNET_H_
