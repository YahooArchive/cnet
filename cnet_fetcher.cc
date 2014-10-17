// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Some portions of this file:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "yahoo/cnet/cnet_fetcher.h"

#include <inttypes.h>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/load_timing_info.h"
#include "net/base/net_errors.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_file_element_reader.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"
#include "yahoo/cnet/cnet_mime.h"
#include "yahoo/cnet/cnet_oauth.h"
#include "yahoo/cnet/cnet_pool.h"
#include "yahoo/cnet/cnet_response.h"
#include "yahoo/cnet/cnet_url_params.h"

#if defined(OS_ANDROID)
#include "net/proxy/proxy_config_service_android.h"
#endif

namespace {

int kMaxBodyPrealloc = 5*1024*1024;
int kBodyIncrement = 16*1024;

// URLRequestJob assumes constrained read sizes less than 1e6 bytes.
int kReadIncrement = 64*1024;

int kUploadProgressIntervalMs = 100;
int kMinSpeedIntervalMs = 1000;

} // namespace


namespace cnet {

void FetcherTraits::Destruct(const Fetcher* fetcher) {
  fetcher->OnDestruct();
}

// TODO: stop using -1 for unknown transfer sizes

Fetcher::Fetcher(scoped_refptr<Pool> pool, const std::string& url,
    const std::string& method, CompletionCallback completion,
    ProgressCallback download, ProgressCallback upload)
    : pool_(pool), initial_url_(url), gurl_(url), method_(method),
      cache_behavior_(CACHE_NORMAL), stop_on_redirect_(false),
      params_encoding_(ENCODE_URL),
      upload_range_offset_(0), upload_range_length_(kuint64max),
      completion_(completion), download_callback_(download),
      upload_callback_(upload),
      redirect_status_code_(-1), was_redirected_(false),
      expected_bytes_(-1), received_bytes_(0),
      pending_files_ops_(0), output_failure_(false),
      min_speed_bytes_sec_(0), min_speed_coefficient_(0.4),
      last_progress_bytes_(0), last_bytes_sec_(0),
      user_data_(NULL) {
  CHECK(pool_.get() != NULL);
}

Fetcher::~Fetcher() {
  DCHECK(pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread());
}

void Fetcher::OnDestruct() const {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::OnDestruct, base::Unretained(this)));
    return;
  }

  delete this;
}

void Fetcher::SetHeader(const std::string& key, const std::string& value) {
  headers_[key] = value;
}

void Fetcher::SetOauthCredentials(const OauthCredentials& credentials) {
  oauth_credentials_.reset(new OauthCredentials());
  *oauth_credentials_ = credentials;
}

void Fetcher::SetCacheBehavior(cnet::Fetcher::CacheBehavior behavior) {
  cache_behavior_ = behavior;
}

void Fetcher::SetStopOnRedirect(bool stop_on_redirect) {
  stop_on_redirect_ = stop_on_redirect;
}

void Fetcher::SetUrlParamsEncoding(cnet::Fetcher::UrlParamsEncoding encoding) {
  params_encoding_ = encoding;
  if (params_encoding_ != ENCODE_URL) {
    upload_body_.clear();
    upload_content_type_.clear();
    upload_file_path_.clear();
  }
}

void Fetcher::SetUrlParam(const std::string& key, const std::string& value) {
  url_params_[key] = value;
}

void Fetcher::SetUrlParamFile(
    const std::string& key, const std::string& filename,
    const std::string& content_type, const base::FilePath& file_path,
    uint64 range_offset, uint64 range_length) {
  // Avoid contradictory parameter settings.
  params_encoding_ = ENCODE_BODY_MULTIPART;
  upload_body_.clear();

  // Set the upload parameters.
  upload_param_key_ = key;
  upload_filename_ = filename;
  upload_content_type_ = content_type;
  upload_file_path_ = file_path;
  upload_range_offset_ = range_offset;
  upload_range_length_ = range_length;
}

void Fetcher::SetUploadBody(const std::string &content_type,
    const std::string &body) {
  // Avoid contradictory parameter settings.
  params_encoding_ = ENCODE_URL;
  upload_file_path_.clear();
  upload_filename_.clear();
  upload_param_key_.clear();

  // Set the upload parameters.
  upload_content_type_ = content_type;
  upload_body_ = body;
}

void Fetcher::SetUploadFilePath(
    const std::string& content_type,
    const base::FilePath& file_path,
    uint64 range_offset,
    uint64 range_length) {
  // Avoid contradictory parameter settings.
  params_encoding_ = ENCODE_URL;
  upload_body_.clear();
  upload_filename_.clear();
  upload_param_key_.clear();

  // Set the upload parameters.
  upload_content_type_ = content_type;
  upload_file_path_ = file_path;
  upload_range_offset_ = range_offset;
  upload_range_length_ = range_length;
}

void Fetcher::SetOutputFilePath(const base::FilePath &file_path) {
  output_path_ = file_path;
}

void Fetcher::SetMinSpeed(double bytes_sec, double duration_secs) {
  if (duration_secs == 0) {
    min_speed_bytes_sec_ = 0;
    min_speed_coefficient_ = 0.4;
  } else {
    if (duration_secs < kMinSpeedIntervalMs/1000.0) {
      duration_secs = kMinSpeedIntervalMs/1000.0;
    }
    min_speed_bytes_sec_ = bytes_sec;
    min_speed_coefficient_ = 1.0/duration_secs;
  }
}

bool Fetcher::BuildRequest() {
  DCHECK(request_ == NULL);

  if (!gurl_.is_valid()) {
    LOG(ERROR) << "Invalid URL: " << gurl_.possibly_invalid_spec();
    return false;
  }

  // Prepare the URL.
  if (oauth_credentials_ != NULL) {
    OauthSignRequest(*oauth_credentials_, gurl_, method_, url_params_);
  }
  if ((url_params_.size() > 0) && (params_encoding_ == ENCODE_URL)) {
    std::string query = OauthCompatibleEncodeParams(url_params_);
    if (gurl_.has_query()) {
      DCHECK(oauth_credentials_ == NULL);
      if (oauth_credentials_ != NULL) {
        LOG(ERROR) << "unimplemented; oauth will likely fail";
      }
      query = gurl_.query() + "&" + query;
    }
    GURL::Replacements replacements;
    replacements.SetQueryStr(query);
    gurl_ = gurl_.ReplaceComponents(replacements);
  }

  // Create the request.
  net::URLRequestContextGetter* url_context_getter =
      pool_->pool_context_getter().get();
  CHECK(url_context_getter != NULL);
  request_ = url_context_getter->GetURLRequestContext()->CreateRequest(gurl_,
      net::DEFAULT_PRIORITY, this, NULL);
  if (request_ != NULL) {
    // Configure load flags.
    int flags = net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_DO_NOT_SEND_COOKIES;
    if (pool_->trust_all_cert_authorities()) {
      flags |= net::LOAD_IGNORE_CERT_AUTHORITY_INVALID;
    }
    switch (cache_behavior_) {
      case CACHE_NORMAL: break;
      case CACHE_BYPASS: flags |= net::LOAD_BYPASS_CACHE; break;
      case CACHE_DISABLE: flags |= net::LOAD_DISABLE_CACHE; break;
      case CACHE_IF_OFFLINE: flags |= net::LOAD_FROM_CACHE_IF_OFFLINE; break;
      case CACHE_ONLY: flags |= net::LOAD_ONLY_FROM_CACHE; break;
      case CACHE_PREFER: flags |= net::LOAD_PREFERRING_CACHE; break;
      case CACHE_VALIDATE: flags |= net::LOAD_VALIDATE_CACHE; break;
    }
    request_->SetLoadFlags(flags);

    request_->set_method(method_);

    // Set additional headers.
    for (Headers::const_iterator it = headers_.begin();
         it != headers_.end(); ++it) {
      request_->SetExtraRequestHeaderByName(it->first, it->second, true);
    }

    // Configure the POST body.
    if ((url_params_.size() > 0) && (params_encoding_ != ENCODE_URL)) {
      if (params_encoding_ == ENCODE_BODY_URL) {
        // Send URL-encoded parameters as the body.
        upload_body_ = OauthCompatibleEncodeParams(url_params_);
        request_->SetExtraRequestHeaderByName(
            net::HttpRequestHeaders::kContentType,
                "application/x-www-form-urlencoded", true);
        request_->SetExtraRequestHeaderByName(
            net::HttpRequestHeaders::kContentLength,
            base::IntToString(upload_body_.size()), true);
      } else if (params_encoding_ == ENCODE_BODY_MULTIPART) {
        // Send a multi-part form as the body.
        std::string mime_boundary = mime::GenerateMimeBoundary();

        upload_body_.clear();
        for (UrlParams::const_iterator it = url_params_.begin();
             it != url_params_.end(); ++it) {
          mime::AddMultipartValueForPost(it->first, it->second,
              mime_boundary, &upload_body_);
        }

        if (upload_file_path_.empty()) {
          mime::AddMultipartFinalDelimiterForPost(mime_boundary, &upload_body_);

          scoped_ptr<net::UploadElementReader> reader(
              new net::UploadBytesElementReader(
                  upload_body_.data(), upload_body_.size()));

          request_->SetExtraRequestHeaderByName(
              net::HttpRequestHeaders::kContentType,
              "multipart/form-data; boundary=" + mime_boundary, true);
          request_->SetExtraRequestHeaderByName(
              net::HttpRequestHeaders::kContentLength,
              base::IntToString(upload_body_.size()), true);
          request_->set_upload(make_scoped_ptr(
              net::UploadDataStream::CreateWithReader(reader.Pass(), 0)));
        } else {
          // Add a file upload to the multi-part form.
          mime::StartMultipartValueForPost(upload_param_key_, "filename",
              upload_filename_, upload_content_type_,
              mime_boundary, &upload_body_);

          upload_body_terminator_.clear();
          mime::FinishMultipartValueForPost(&upload_body_terminator_);
          mime::AddMultipartFinalDelimiterForPost(mime_boundary,
              &upload_body_terminator_);

          ScopedVector<net::UploadElementReader> readers;
          readers.push_back(
              new net::UploadBytesElementReader(
                  upload_body_.data(), upload_body_.size()));
          readers.push_back(
              new net::UploadFileElementReader(pool_->GetFileTaskRunner().get(),
                  upload_file_path_, upload_range_offset_,
                  upload_range_length_, base::Time()));
          readers.push_back(
              new net::UploadBytesElementReader( upload_body_terminator_.data(),
                  upload_body_terminator_.size()));

          request_->SetExtraRequestHeaderByName(
              net::HttpRequestHeaders::kContentType,
              "multipart/form-data; boundary=" + mime_boundary, true);
          request_->set_upload(make_scoped_ptr(
              new net::UploadDataStream(readers.Pass(), 0)));
        }
      }
    } else if (!upload_body_.empty()) {
      // Send a raw POST body.
      scoped_ptr<net::UploadElementReader> reader(
          new net::UploadBytesElementReader(
              upload_body_.data(), upload_body_.size()));

      if (!upload_content_type_.empty()) {
        request_->SetExtraRequestHeaderByName(
            net::HttpRequestHeaders::kContentType, upload_content_type_, true);
      }
      request_->SetExtraRequestHeaderByName(
          net::HttpRequestHeaders::kContentLength,
          base::IntToString(upload_body_.size()), true);
      request_->set_upload(make_scoped_ptr(
          net::UploadDataStream::CreateWithReader(reader.Pass(), 0)));
    } else if (!upload_file_path_.empty()) {
      // Send a file as the POST body.
      scoped_ptr<net::UploadElementReader> reader(
          new net::UploadFileElementReader(pool_->GetFileTaskRunner().get(),
              upload_file_path_, upload_range_offset_,
              upload_range_length_, base::Time()));

      if (!upload_content_type_.empty()) {
        request_->SetExtraRequestHeaderByName(
            net::HttpRequestHeaders::kContentType, upload_content_type_, true);
      }
      request_->set_upload(make_scoped_ptr(
          net::UploadDataStream::CreateWithReader(reader.Pass(), 0)));
    }

    return true;
  } else {
    return false;
  }
}

void Fetcher::Start() {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::Start, this));
    return;
  }

  DCHECK(request_started_.is_null());
  if (!request_started_.is_null()) {
    return;
  }
  request_started_ = base::TimeTicks::Now();

  this->AddRef(); // Stay alive until the request completes.
  pool_->FetcherStarting(this); // Claim pool resources.

  if (BuildRequest()) {
    request_->Start();
  } else {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::OnRequestComplete, this));
  }
}

void Fetcher::Cancel() {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::Cancel, this));
    return;
  }

  if (!network_started_.is_null()) {
    // TODO: implement cancel suppression.
  }
  if (!request_started_.is_null()) {
    if (request_ != NULL) {
      request_->Cancel();
    }
    OnRequestComplete();
  }
}

void Fetcher::OnBeforeNetworkStart(net::URLRequest* request, bool* defer) {
  network_started_ = base::TimeTicks::Now();

  if ((request_->get_upload() != NULL) && !upload_callback_.is_null()) {
    upload_progress_timer_.reset(new base::RepeatingTimer<Fetcher>());
    upload_progress_timer_->Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(kUploadProgressIntervalMs),
        this, &Fetcher::OnUploadProgressTimer);
  }
  if (min_speed_bytes_sec_ > 0) {
    min_speed_timer_.reset(new base::RepeatingTimer<Fetcher>());
    min_speed_timer_->Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(kMinSpeedIntervalMs),
        this, &Fetcher::OnMinSpeedTimer);
  }
}

void Fetcher::OnUploadProgressTimer() {
  if ((request_ != NULL) && request_->is_pending()) {
    net::UploadProgress progress = request_->GetUploadProgress();
    if (!upload_callback_.is_null()) {
      uint64 position = progress.position();
      uint64 total = progress.size();
      pool_->GetWorkTaskRunner()->PostTask(FROM_HERE,
          base::Bind(upload_callback_, make_scoped_refptr(this),
                     position, total));
      if ((total > 0) && (position >= total)) {
        // We don't need the upload timer firing once we've finished
        // the upload.
        upload_progress_timer_.reset();
      }
    }
  }
}

void Fetcher::OnMinSpeedTimer() {
  if ((request_ != NULL) && request_->is_pending()) {
    // Compute a very rough estimate of our bandwidth.
    net::UploadProgress progress = request_->GetUploadProgress();
    int64 bytes = progress.position() + request_->GetTotalReceivedBytes();

    double current_bytes_sec =
        (double)(bytes-last_progress_bytes_)/(double)kMinSpeedIntervalMs;

    double avg_bytes_sec;
    if (last_bytes_sec_ > 0) {
      avg_bytes_sec = (1-min_speed_coefficient_)*last_bytes_sec_ +
                      min_speed_coefficient_*current_bytes_sec;
    } else {
      avg_bytes_sec = current_bytes_sec;
    }

    last_progress_bytes_ = bytes;
    last_bytes_sec_ = avg_bytes_sec;

    base::TimeDelta delta = base::TimeTicks::Now() - request_started_;
    if ((double)delta.InSeconds()*min_speed_coefficient_ > 1.0) {
      if (avg_bytes_sec < min_speed_bytes_sec_) {
        request_->CancelWithError(net::ERR_TIMED_OUT);
        OnRequestComplete();
      }
    }
  }
}

void Fetcher::OnReceivedRedirect(net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  *defer_redirect = false;
  if (stop_on_redirect_) {
    was_redirected_ = true;
    redirect_status_code_ = redirect_info.status_code;
    request_->Cancel();
    OnRequestComplete();
  }
}

void Fetcher::OnResponseStarted(net::URLRequest* request) {
  if (request->status().status() != net::URLRequestStatus::SUCCESS) {
    OnRequestComplete();
  } else {
    receive_started_ = base::TimeTicks::Now();
    expected_bytes_ = request->GetExpectedContentSize();

    if (output_path_.empty()) {
      if (read_buffer_.get() == NULL) {
        read_buffer_ = new net::GrowableIOBuffer();
      }

      if (expected_bytes_ > 0) {
        int prealloc = (expected_bytes_ < kMaxBodyPrealloc) ?
            expected_bytes_:kMaxBodyPrealloc;
        read_buffer_->SetCapacity(prealloc);
      }

      ReadIntoBufferStart();
    } else {
      // Add a pending op for starting the request.  It will be matched
      // in OnRequestComplete() by a decrement.
      pending_files_ops_++;

      pending_files_ops_++; // For the file open.
      FileOpen();

      ReadIntoFileStart();
    }
  }
}

void Fetcher::OnReadCompleted(net::URLRequest* request, int bytes_read) {
  if (bytes_read <= 0) {
    OnRequestComplete();
  } else if (output_path_.empty()) {
    ReadIntoBufferComplete(bytes_read);
    ReadIntoBufferStart();
  } else {
    ReadIntoFileComplete(bytes_read);
    ReadIntoFileStart();
  }
}

void Fetcher::OnRequestComplete() {
  if (!receive_completed_.is_null()) {
    return;
  }
  receive_completed_ = base::TimeTicks::Now();

  upload_progress_timer_.reset();
  min_speed_timer_.reset();

  if (output_path_.empty()) {
    FinishRequest();
  } else {
    FileCompleteOp();
  }
}

void Fetcher::OnDownloadProgress(int64 progress, int64 expected) {
  if (!download_callback_.is_null()) {
    pool_->GetWorkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(download_callback_, make_scoped_refptr(this),
                   progress, expected));
  }
}

// LICENSE: modeled after URLRequestAdapter::Read() from
//     components/cronet/android/url_request_adapter.cc
void Fetcher::ReadIntoBufferStart() {
  CHECK(read_buffer_.get() != NULL);
  while (true) {
    if (read_buffer_->RemainingCapacity() == 0) {
      int64 new_capacity = read_buffer_->capacity() + kBodyIncrement;
      if (new_capacity > kint32max) {
        // The fetcher is not designed to handle large responses.
        request_->Cancel();
        OnRequestComplete();
        break;
      }
      read_buffer_->SetCapacity((int)new_capacity);
    }

    int bytes_read;
    int to_read = std::min(kReadIncrement, read_buffer_->RemainingCapacity());
    if (request_->Read(read_buffer_.get(), to_read, &bytes_read)) {
      // We completed a synchronous read.
      if (bytes_read == 0) {
        OnRequestComplete();
        break;
      }
      ReadIntoBufferComplete(bytes_read);
    } else if (request_->status().status() ==
               net::URLRequestStatus::IO_PENDING) {
      // We've started an asynchronous read.
      break;
    } else {
      OnRequestComplete();
      break;
    }
  }
}

void Fetcher::ReadIntoBufferComplete(int bytes_read) {
  CHECK(read_buffer_.get() != NULL);
  read_buffer_->set_offset(read_buffer_->offset() + bytes_read);

  received_bytes_ += bytes_read;
  OnDownloadProgress(received_bytes_, expected_bytes_);
}

void Fetcher::ReadIntoFileStart() {
  while (true) {
    read_buffer_ = new net::GrowableIOBuffer();
    read_buffer_->SetCapacity(kReadIncrement);

    int bytes_read;
    if (request_->Read(read_buffer_.get(), read_buffer_->RemainingCapacity(),
                        &bytes_read)) {
      if (bytes_read == 0) {
        OnRequestComplete();
        break;
      }
      ReadIntoFileComplete(bytes_read);
    } else if (request_->status().status() ==
               net::URLRequestStatus::IO_PENDING) {
      // We've started an asynchronous read.
      break;
    } else {
      OnRequestComplete();
      break;
    }
  }
}

void Fetcher::ReadIntoFileComplete(int bytes_read) {
  CHECK(read_buffer_.get() != NULL);
  scoped_refptr<net::IOBuffer> buffer = read_buffer_;
  read_buffer_ = NULL;

  pending_files_ops_++;
  FileChunkWrite(buffer, bytes_read);

  received_bytes_ += bytes_read;
  OnDownloadProgress(received_bytes_, expected_bytes_);
}

void Fetcher::FileOpen() {
  if (!pool_->GetFileTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetFileTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::FileOpen, this));
    return;
  }

  DCHECK(output_file_ == NULL);
  if (output_file_ == NULL) {
    output_file_.reset(new base::File());
    output_file_->InitializeUnsafe(output_path_,
        base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
    if (!output_file_->IsValid()) {
      LOG(ERROR) << "Error creating: " << output_path_.value() << " : "
                 << output_file_->ErrorToString(output_file_->error_details());
    }
  }
  OnFileOpened(output_file_->IsValid());
}

void Fetcher::OnFileOpened(bool success) {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::OnFileOpened, this, success));
    return;
  }

  if (!success) {
    output_failure_ = true;
    Cancel();
  }
  FileCompleteOp();
}

void Fetcher::FileChunkWrite(scoped_refptr<net::IOBuffer> buffer,
    int length) {
  if (!pool_->GetFileTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetFileTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::FileChunkWrite, this, buffer, length));
    return;
  }

  int bytes_written = -1;
  if ((output_file_ != NULL) && output_file_->IsValid()) {
    bytes_written = output_file_->WriteAtCurrentPos(buffer->data(), length);
    if (bytes_written >= 0) {
      DCHECK(bytes_written == length);
      if (bytes_written != length) {
        bytes_written = -1;
      }
    }
  }
  OnFileChunkWritten(bytes_written);
}

void Fetcher::OnFileChunkWritten(int bytes_written) {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::OnFileChunkWritten, this, bytes_written));
    return;
  }

  if (bytes_written < 0) {
    output_failure_ = true;
    Cancel();
  }
  FileCompleteOp();
}

void Fetcher::FileCompleteOp() {
  DCHECK(pending_files_ops_ > 0);
  if (pending_files_ops_ > 0) {
    pending_files_ops_--;
    if (pending_files_ops_ == 0) {
// TODO: delete the file if the HTTP connection failed too!
      FileClose(output_failure_);
    }
  }
}

void Fetcher::FileClose(bool remove) {
  if (!pool_->GetFileTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetFileTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::FileClose, this, remove));
    return;
  }

  if ((output_file_ != NULL) && output_file_->IsValid()) {
    output_file_->Close();

    if (remove && !output_path_.empty()) {
      if (!base::DeleteFile(output_path_, false)) {
        LOG(ERROR) << "Failed to cleanup file: " << output_path_.value();
      }
    }
  }

  OnFileClosed();
}

void Fetcher::OnFileClosed() {
  if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
    pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(&Fetcher::OnFileClosed, this));
    return;
  }

  FinishRequest();
}

void Fetcher::ConvertTiming(CnetLoadTiming *cnet_timing,
    int http_response_code, int64 content_len) {
  net::LoadTimingInfo net_timing;
  request_->GetLoadTimingInfo(&net_timing);
  cnet_timing->start_s = net_timing.request_start_time.ToJsTime();
  cnet_timing->socket_reused = net_timing.socket_reused;
  cnet_timing->socket_log_id = net_timing.socket_log_id;

  if (!request_started_.is_null() && receive_completed_ > request_started_) {
    base::TimeDelta delta = receive_completed_ - request_started_;
    cnet_timing->total_ms = delta.InMilliseconds();
  } else {
    cnet_timing->total_ms = 0;
  }

  if (!net_timing.connect_timing.dns_start.is_null() &&
      net_timing.connect_timing.dns_end > net_timing.connect_timing.dns_start) {
    base::TimeDelta delta =
        net_timing.connect_timing.dns_end - net_timing.connect_timing.dns_start;
    cnet_timing->dns_ms = delta.InMilliseconds();
  } else {
    cnet_timing->dns_ms = 0;
  }

  if (!net_timing.connect_timing.ssl_start.is_null() &&
      net_timing.connect_timing.ssl_end > net_timing.connect_timing.ssl_start) {
    base::TimeDelta delta = net_timing.connect_timing.ssl_end -
        net_timing.connect_timing.ssl_start;
    cnet_timing->ssl_ms = delta.InMilliseconds();
  } else {
    cnet_timing->ssl_ms = 0;
  }

  if (!net_timing.connect_timing.connect_start.is_null() &&
      (net_timing.connect_timing.connect_end >
       net_timing.connect_timing.connect_start)) {
    base::TimeDelta delta =
        net_timing.connect_timing.connect_end -
            net_timing.connect_timing.connect_start;
    cnet_timing->connect_ms = delta.InMilliseconds();
    if (cnet_timing->connect_ms > cnet_timing->ssl_ms) {
      cnet_timing->connect_ms -= cnet_timing->ssl_ms;
    }
  } else {
    cnet_timing->connect_ms = 0;
  }

  if (!net_timing.proxy_resolve_start.is_null() &&
      net_timing.proxy_resolve_end > net_timing.proxy_resolve_start) {
    base::TimeDelta delta = net_timing.proxy_resolve_end -
        net_timing.proxy_resolve_start;
    cnet_timing->proxy_resolve_ms = delta.InMilliseconds();
  } else {
    cnet_timing->proxy_resolve_ms = 0;
  }

  if (!net_timing.send_start.is_null() &&
      net_timing.send_end > net_timing.send_start) {
    base::TimeDelta delta = net_timing.send_end - net_timing.send_start;
    cnet_timing->send_ms = delta.InMilliseconds();
  } else {
    cnet_timing->send_ms = 0;
  }

  if (!net_timing.send_start.is_null() &&
      net_timing.receive_headers_end > net_timing.send_end) {
    base::TimeDelta delta = net_timing.receive_headers_end -
        net_timing.send_end;
    cnet_timing->headers_receive_ms = delta.InMilliseconds();
  } else {
    cnet_timing->headers_receive_ms = 0;
  }

  if (!receive_started_.is_null()) {
    base::TimeDelta delta = receive_completed_ - receive_started_;
    cnet_timing->data_receive_ms = delta.InMilliseconds();
  } else {
    cnet_timing->data_receive_ms = 0;
  }

  unsigned sum = cnet_timing->proxy_resolve_ms + cnet_timing->dns_ms +
      cnet_timing->connect_ms + cnet_timing->ssl_ms + cnet_timing->send_ms +
      cnet_timing->headers_receive_ms + cnet_timing->data_receive_ms;
  if (cnet_timing->total_ms > sum) {
    cnet_timing->queued_ms = cnet_timing->total_ms - sum;
  } else {
    cnet_timing->queued_ms = 0;
  }

  cnet_timing->from_cache = request_->was_cached();
  cnet_timing->total_recv_bytes = request_->GetTotalReceivedBytes();
  cnet_timing->total_send_bytes = 0;

  if (pool_->log_level() > 1) {
    GURL url(request_->url());
    std::string log = base::StringPrintf(
        "(stats) startMs=%" PRIu64 " conn=%u"
        " reused=%d timeMs=%u status=%d server=%s downBytes=%" PRIu64
        " contentBytes=%" PRIu64 " queuedMs=%u dnsMs=%u connectMs=%u"
        " sslMs=%u sendMs=%u firstByteMs=%u receiveMs=%u url=%s",
        (uint64_t)(cnet_timing->start_s),
        cnet_timing->socket_log_id,
        cnet_timing->socket_reused,
        cnet_timing->total_ms,
        http_response_code,
        url.host().c_str(), cnet_timing->total_recv_bytes, content_len,
        cnet_timing->queued_ms,
        cnet_timing->dns_ms,
        cnet_timing->connect_ms + cnet_timing->proxy_resolve_ms,
        cnet_timing->ssl_ms,
        cnet_timing->send_ms,
        cnet_timing->headers_receive_ms,
        cnet_timing->data_receive_ms,
        url.spec().c_str());
    LOG(INFO) << log;
  }
}

void Fetcher::FinishRequest() {
  scoped_ptr<net::HttpResponseInfo> response_info;
  net::HttpResponseHeaders* response_headers = NULL;
  scoped_ptr<CnetLoadTiming> cnet_timing(new CnetLoadTiming);
  net::URLRequestStatus status(net::URLRequestStatus::FAILED, net::ERR_FAILED);
  int http_response_code = -1;
  if (request_ != NULL) {
    if (!output_failure_) {
      response_info.reset(new net::HttpResponseInfo(request_->response_info()));
      response_headers = request_->response_headers();
      status = request_->status();
      if (status.status() == net::URLRequestStatus::Status::SUCCESS) {
        http_response_code = request_->GetResponseCode();
        if ((expected_bytes_ > 0) && (expected_bytes_ != received_bytes_) &&
            (pool_->log_level() > 0)) {
          LOG(WARNING) << "Request incomplete <" << request_->url() << ">, "
                       << "expected: " << expected_bytes_
                       << "received: " << received_bytes_;
        }
      } else if (was_redirected_) {
        http_response_code = redirect_status_code_;
        status = net::URLRequestStatus();
      } else if (pool_->log_level() > 0) {
        LOG(ERROR) << "Request failed <" << request_->url()
                   << ">: " << net::ErrorToString(status.error());
      }
    }

    ConvertTiming(cnet_timing.get(), http_response_code, received_bytes_);
  }

  // Ensure that we never invoke the completion again.
  CompletionCallback completion = completion_;
  completion_.Reset();
  download_callback_.Reset();
  upload_callback_.Reset();

  scoped_refptr<Response> response(new Response(initial_url_, gurl_,
      request_ != NULL ? request_->url():gurl_, read_buffer_,
      url_params_, cnet_timing.Pass(), status, http_response_code,
      response_headers, response_info.Pass()));
  
  if (!completion.is_null()) {
    pool_->GetWorkTaskRunner()->PostTask(FROM_HERE,
        base::Bind(completion, make_scoped_refptr(this), response));
  }

  // Release pool resources.
  pool_->FetcherCompleted(this);

  // This may delete us.
  this->Release();
}

} // namespace cnet
