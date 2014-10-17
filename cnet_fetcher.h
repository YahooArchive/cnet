// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_FETCHER_H_
#define YAHOO_CNET_CNET_FETCHER_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "net/url_request/url_request.h"
#include "yahoo/cnet/cnet.h"
#include "yahoo/cnet/cnet_headers.h"
#include "yahoo/cnet/cnet_url_params.h"

namespace base {
class File;
}

namespace net {
class IOBuffer;
class GrowableIOBuffer;
}


namespace cnet {

class Fetcher;
class OauthCredentials;
class Pool;
class Response;

struct FetcherTraits {
  static void Destruct(const Fetcher* fetcher);
};

class Fetcher : public net::URLRequest::Delegate,
                public base::RefCountedThreadSafe<Fetcher, FetcherTraits> {
 public:
  typedef base::Callback<void(scoped_refptr<Fetcher> fetcher,
      scoped_refptr<Response> response)> CompletionCallback;
  typedef base::Callback<void(scoped_refptr<Fetcher> fetcher,
      int64_t current, int64_t total)> ProgressCallback;

  Fetcher(scoped_refptr<Pool> pool, const std::string& url,
      const std::string& method, CompletionCallback completion,
      ProgressCallback download, ProgressCallback upload);
 
  enum UrlParamsEncoding {
    ENCODE_URL = 0,
    ENCODE_BODY_MULTIPART,
    ENCODE_BODY_URL
  };

  enum CacheBehavior {
    CACHE_NORMAL = 0,

    // Like a browser reload, w/ an if-none-match/if-modified-since query
    CACHE_VALIDATE,

    // Like a browser shift-reload, w/ a "pragma: no-cache" end-to-end fetch
    CACHE_BYPASS,

    // Like browser back/forward; cached content is preferred over protocol-
    // specific cache validation
    CACHE_PREFER,

    // Will fail if the file can't be retrieved from the cache
    CACHE_ONLY,

    // If the request fails due to networking, then behave as if
    // CACHE_PREFER was specified
    CACHE_IF_OFFLINE,

    // Will skip the local cache; it doesn't change the HTTP headers.
    CACHE_DISABLE,
  };

  void SetCacheBehavior(CacheBehavior behavior);
  void SetStopOnRedirect(bool stop_on_redirect);

  void SetHeader(const std::string& key, const std::string& value);

  void SetOauthCredentials(const OauthCredentials& credentials);
  void SetUrlParamsEncoding(UrlParamsEncoding encoding);
  void SetUrlParam(const std::string& key, const std::string& value);
  void SetUrlParamFile(
      const std::string& key, const std::string& filename,
      const std::string& content_type, const base::FilePath& file_path,
      uint64 range_offset, uint64 range_length);

  void SetUploadBody(const std::string& content_type, const std::string& body);

  void SetUploadFilePath(const std::string& content_type,
      const base::FilePath& file_path,
      uint64 range_offset, uint64 range_length);

  void SetOutputFilePath(const base::FilePath& file_path);

  void SetMinSpeed(double bytes_sec, double duration_secs);

  void set_user_data(void* user_data) { user_data_ = user_data; }
  void* get_user_data() { return user_data_; }

  void Start();
  void Cancel();

  scoped_refptr<Pool> pool() { return pool_; }
  const std::string& initial_url() { return initial_url_; }

  // Overrides for URLRequest::Delegate.
  // These are invoked on the original thread that started the request.
  virtual void OnBeforeNetworkStart(net::URLRequest* request,
      bool* defer) override;
  virtual void OnReceivedRedirect(net::URLRequest* request,
      const net::RedirectInfo& redirect_info,
      bool* defer_redirect) override;
  virtual void OnResponseStarted(net::URLRequest* request) override;
  virtual void OnReadCompleted(net::URLRequest* request,
      int bytes_read) override;

 private:
  bool BuildRequest();

  void OnUploadProgressTimer();
  void OnMinSpeedTimer();

  void ReadIntoBufferStart();
  void ReadIntoBufferComplete(int bytes_read);

  void FileOpen();
  void OnFileOpened(bool success);
  void ReadIntoFileStart();
  void ReadIntoFileComplete(int bytes_read);
  void FileChunkWrite(scoped_refptr<net::IOBuffer> buffer, int length);
  void OnFileChunkWritten(int bytes_written);
  void FileCompleteOp();
  void FileClose(bool remove);
  void OnFileClosed();

  void ConvertTiming(CnetLoadTiming *cnet_timing,
      int http_response_code, int64 content_len);

  void OnDownloadProgress(int64 progress, int64 expected);
  void OnRequestComplete();
  void FinishRequest();

  void OnDestruct() const;

  scoped_refptr<Pool> pool_;
  scoped_ptr<net::URLRequest> request_;

  std::string initial_url_;
  GURL gurl_;
  std::string method_;
  CacheBehavior cache_behavior_;
  bool stop_on_redirect_;
  Headers headers_;
  UrlParamsEncoding params_encoding_;
  scoped_ptr<OauthCredentials> oauth_credentials_;
  UrlParams url_params_;
  std::string upload_body_;
  std::string upload_body_terminator_;
  std::string upload_content_type_;

  std::string upload_filename_;
  std::string upload_param_key_;
  base::FilePath upload_file_path_;
  uint64 upload_range_offset_;
  uint64 upload_range_length_;

  CompletionCallback completion_;
  ProgressCallback download_callback_;
  ProgressCallback upload_callback_;

  scoped_ptr<base::RepeatingTimer<Fetcher> > upload_progress_timer_;
  base::TimeTicks request_started_;
  base::TimeTicks receive_started_;
  base::TimeTicks receive_completed_;
  base::TimeTicks network_started_;
  int redirect_status_code_;
  bool was_redirected_;
  int64 expected_bytes_;
  int64 received_bytes_;
  base::FilePath output_path_;
  scoped_ptr<base::File> output_file_;
  int pending_files_ops_;
  bool output_failure_;
  scoped_refptr<net::GrowableIOBuffer> read_buffer_;

  scoped_ptr<base::RepeatingTimer<Fetcher> > min_speed_timer_;
  double min_speed_bytes_sec_;
  double min_speed_coefficient_;
  int64 last_progress_bytes_;
  double last_bytes_sec_;

  void* user_data_;

  virtual ~Fetcher();
  friend class base::RefCountedThreadSafe<Fetcher>;
  friend struct FetcherTraits;
  DISALLOW_COPY_AND_ASSIGN(Fetcher);
};

} // namespace cnet

#endif  // YAHOO_CNET_CNET_FETCHER_H_
