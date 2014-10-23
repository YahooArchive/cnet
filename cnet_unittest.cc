// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Some portions of this file:
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/statistics_recorder.h"
#include "base/run_loop.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "net/http/http_response_headers.h"
#include "net/socket/client_socket_pool_base.h"
#include "net/socket/ssl_server_socket.h"
#include "net/test/net_test_suite.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "yahoo/cnet/cnet.h"
#include "yahoo/cnet/cnet_fetcher.h"
#include "yahoo/cnet/cnet_pool.h"
#include "yahoo/cnet/cnet_response.h"

using net::internal::ClientSocketPoolBaseHelper;

// Copied from url_request_unittest.cc
class LocalHttpTestServer : public net::SpawnedTestServer {
 public:
  explicit LocalHttpTestServer(const base::FilePath& document_root)
      : net::SpawnedTestServer(net::SpawnedTestServer::TYPE_HTTPS,
            "127.0.0.1", document_root) {}
  LocalHttpTestServer()
      : net::SpawnedTestServer(net::SpawnedTestServer::TYPE_HTTPS,
            "127.0.0.1", base::FilePath()) {}
};

class PoolTest : public PlatformTest {
 public:
  PoolTest()
      : quit_event_(false, false) {
    base::Thread::Options options;
    options.message_loop_type = base::MessageLoop::TYPE_UI;
    ui_thread_ = new base::Thread("cnet-ui");
    ui_thread_->StartWithOptions(options);

    cnet::Pool::Config config;
    config.user_agent = "cnet-unittest";
    pool_ = new cnet::Pool(ui_thread_->task_runner(), config);
  }

  virtual ~PoolTest() { }

  virtual void TearDown() override {
    StartDelete();
    quit_event_.Wait();
    ui_thread_->Stop();
  }

 protected:
  void StartDelete() {
    if (!pool_->GetNetworkTaskRunner()->RunsTasksOnCurrentThread()) {
      pool_->GetNetworkTaskRunner()->PostTask(FROM_HERE,
         base::Bind(&PoolTest::StartDelete, base::Unretained(this)));
      return;
    }

    // This should post a task to the UI thread to delete the pool's threads.
    // We want to do this from the network thread so that we know that
    // it will post a task to the UI thread before us.
    // This works only if the retain count is currently 1.
    pool_ = NULL;

    // Now we post a task to the UI thread to follow the cleanup of the pool.
    ui_thread_->message_loop()->PostTask(FROM_HERE,
        base::Bind(&PoolTest::OnPoolDeleted, base::Unretained(this)));
  }

  void OnPoolDeleted() {
    ASSERT_TRUE(ui_thread_->task_runner()->RunsTasksOnCurrentThread());
    quit_event_.Signal();
  }

  base::Thread* ui_thread_;
  base::WaitableEvent quit_event_;
  scoped_refptr<cnet::Pool> pool_;
};

TEST_F(PoolTest, InitShutdown) {
}


class FetcherTest : public PoolTest {
 public:
  FetcherTest()
      : completed_event_(false, false),
        test_server_(base::FilePath(FILE_PATH_LITERAL(
            "yahoo/cnet/data/cnet_unittest"))),
        download_progress_(0), upload_progress_(0) {
  }
 
  void OnFetcherCompleted(scoped_refptr<cnet::Fetcher> fetcher,
      scoped_refptr<cnet::Response> response) {
    // On work thread.
    response_ = response;
    completed_event_.Signal();
  }

  void OnFetcherDownloadProgress(scoped_refptr<cnet::Fetcher> fetcher,
      int64_t current, int64_t total) {
    // On work thread.
    download_progress_ = current;
  }
  
  void OnFetcherUploadProgress(scoped_refptr<cnet::Fetcher> fetcher,
      int64_t current, int64_t total) {
    // On work thread.
    upload_progress_ = current;
  }

 protected:
  scoped_refptr<cnet::Response> WaitForCompletion() {
    completed_event_.Wait();
    return response_;
  }

  base::WaitableEvent completed_event_;
  LocalHttpTestServer test_server_;
  scoped_refptr<cnet::Response> response_;
  int64 download_progress_;
  int64 upload_progress_;
};

TEST_F(FetcherTest, SimpleFetch) {
  ASSERT_TRUE(test_server_.Start());

  std::string url(test_server_.GetURL("files/hello.html").spec());
  scoped_refptr<cnet::Fetcher> fetcher(new cnet::Fetcher(
      pool_, url, "GET",
      base::Bind(&FetcherTest::OnFetcherCompleted, base::Unretained(this)),
      base::Bind(&FetcherTest::OnFetcherDownloadProgress,
          base::Unretained(this)),
      base::Bind(&FetcherTest::OnFetcherUploadProgress,
          base::Unretained(this))));
  fetcher->Start();

  scoped_refptr<cnet::Response> response = WaitForCompletion();
  ASSERT_EQ(response->status().status(), net::URLRequestStatus::SUCCESS);
  ASSERT_EQ(response->http_response_code(), 200);
  EXPECT_EQ(response->initial_url(), url);
  EXPECT_EQ(response->original_url(), GURL(url));
  EXPECT_EQ(response->final_url(), GURL(url));

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      response->response_headers());
  EXPECT_TRUE(response_headers->HasHeaderValue("Content-Type", "text/html"));

  std::string expected_body("Hello!\n\n");
  ASSERT_EQ((int)expected_body.length(), response->response_length());
  std::string response_body(response->response_body(),
      response->response_length());
  ASSERT_EQ(expected_body, response_body);

  EXPECT_EQ(upload_progress_, 0);
  EXPECT_EQ(download_progress_, response->response_length());
}

TEST_F(FetcherTest, FetchError) {
  ASSERT_TRUE(test_server_.Start());

  std::string url(test_server_.GetURL("close-socket").spec());
  scoped_refptr<cnet::Fetcher> fetcher(new cnet::Fetcher(
      pool_, url, "GET",
      base::Bind(&FetcherTest::OnFetcherCompleted, base::Unretained(this)),
      base::Bind(&FetcherTest::OnFetcherDownloadProgress,
          base::Unretained(this)),
      base::Bind(&FetcherTest::OnFetcherUploadProgress,
          base::Unretained(this))));
  fetcher->Start();

  scoped_refptr<cnet::Response> response = WaitForCompletion();
  ASSERT_NE(response->status().status(), net::URLRequestStatus::SUCCESS);
  ASSERT_EQ(response->status().status(), net::URLRequestStatus::FAILED);
  ASSERT_EQ(response->http_response_code(), -1);
}

int main(int argc, char** argv) {
  base::StatisticsRecorder::Initialize();

  NetTestSuite test_suite(argc, argv);
  ClientSocketPoolBaseHelper::set_connect_backup_jobs_enabled(false);

#if defined(OS_WIN)
  // We want to be sure to init NSPR on the main thread.
  crypto::EnsureNSPRInit();
#endif

  // Enable support for SSL server sockets, which must be done while
  // single-threaded.
  net::EnableSSLServerSockets();

  return base::LaunchUnitTests(argc, argv,
      base::Bind(&NetTestSuite::Run, base::Unretained(&test_suite)));
}
