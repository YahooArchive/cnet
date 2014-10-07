// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_POOL_H_
#define YAHOO_CNET_CNET_POOL_H_

#include <set>
#include <map>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread.h"
#include "net/url_request/url_request_context_getter.h"

namespace net {
class NetworkChangeNotifier;
class ProxyConfigService;
class URLRequestContext;
}


namespace cnet {

class Fetcher;
class ProxyConfigService;
class Pool;

struct PoolTraits {
  static void Destruct(const Pool* pool);
};

class Pool : public base::RefCountedThreadSafe<Pool, PoolTraits> {
 public:
  struct Config {
    Config();
    ~Config();

    std::string user_agent;

    bool enable_spdy;
    bool enable_quic;

    bool enable_ssl_false_start;
    bool trust_all_cert_authorities;
    bool disable_system_proxy;

    base::FilePath cache_path;
    unsigned cache_max_bytes;

    int log_level;
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Fires on the pool's network thread when there are no more outstanding
    // requests.
    virtual void OnPoolIdle(scoped_refptr<Pool> pool) = 0;
  };

  Pool(scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      const Config& config);

  void SetProxyConfig(const std::string& rules);

  void SetTrustAllCertAuthorities(bool value);
  bool trust_all_cert_authorities() { return trust_all_cert_authorities_; }

  void SetEnableSslFalseStart(bool value);
  bool get_ssl_false_start() { return enable_ssl_false_start_; }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void Preconnect(const std::string& url, int num_streams);

  // TODO: move the add-tag logic into an observer on the fetcher.
  void TagFetcher(scoped_refptr<Fetcher> fetcher, int tag);
  void CancelTag(int tag);

  // TODO: convert these to observers on the fetcher.
  void FetcherStarting(scoped_refptr<Fetcher> fetcher);
  void FetcherCompleted(scoped_refptr<Fetcher> fetcher);

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner() const;
  scoped_refptr<base::SingleThreadTaskRunner> GetWorkTaskRunner() const;
  scoped_refptr<base::SingleThreadTaskRunner> GetFileTaskRunner();

  scoped_refptr<net::URLRequestContextGetter> pool_context_getter() {
    return pool_context_getter_;
  }

  int log_level() { return log_level_; }

  class PoolContextGetter : public net::URLRequestContextGetter {
   public:
    PoolContextGetter(net::URLRequestContext* context,
        scoped_refptr<base::SingleThreadTaskRunner> network_runner);

    // Overrides from net::URLRequestContextGetter
    // Note: GetURLRequestContext() is for use on the network thread.
    virtual net::URLRequestContext* GetURLRequestContext() OVERRIDE;
    virtual scoped_refptr<base::SingleThreadTaskRunner>
        GetNetworkTaskRunner() const OVERRIDE;

   private:
    net::URLRequestContext* context_;
    scoped_refptr<base::SingleThreadTaskRunner> network_runner_;

    virtual ~PoolContextGetter();
    DISALLOW_COPY_AND_ASSIGN(PoolContextGetter);
  };

 private:
  void InitializeURLRequestContext(
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner);
  void OnDestruct() const;
  static void DeleteThreads(base::Thread* network, base::Thread* work,
      base::Thread* file);

  void AllocSystemProxyOnUi();
  void ActivateSystemProxy(net::ProxyConfigService *system_proxy_service);
  
  typedef std::set<scoped_refptr<Fetcher> > FetcherList;
  typedef std::map<int, FetcherList> TagToFetcherList;
  typedef std::map<scoped_refptr<Fetcher>, int> FetcherToTag;

  scoped_refptr<PoolContextGetter> pool_context_getter_;
  scoped_ptr<net::URLRequestContext> context_;
  cnet::ProxyConfigService* proxy_config_service_; // Owned by context_

  scoped_refptr<base::SingleThreadTaskRunner> ui_runner_;
  base::Thread* network_thread_;
  base::Thread* work_thread_;
  base::Thread* file_thread_;
  
  TagToFetcherList tag_to_fetcher_list_;
  FetcherToTag fetcher_to_tag_;
  unsigned outstanding_requests_;

  std::string user_agent_;
  bool enable_spdy_;
  bool enable_quic_;
  bool enable_ssl_false_start_;
  bool disable_system_proxy_;
  base::FilePath cache_path_;
  unsigned cache_max_bytes_;
  bool trust_all_cert_authorities_;
  int log_level_;

  ObserverList<Observer> observers_;

  virtual ~Pool();
  friend class base::RefCountedThreadSafe<Pool>;
  friend struct PoolTraits;
  DISALLOW_COPY_AND_ASSIGN(Pool);
};

} // namespace cnet

#endif  // YAHOO_CNET_CNET_POOL_H_
