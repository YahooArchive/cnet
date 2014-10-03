// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef YAHOO_CNET_CNET_PROXY_SERVICE_H_
#define YAHOO_CNET_CNET_PROXY_SERVICE_H_

#include "net/proxy/proxy_service.h"

namespace cnet {

class ProxyConfigService : public net::ProxyConfigService,
                                 public net::ProxyConfigService::Observer {
 public:
  ProxyConfigService();
  virtual ~ProxyConfigService();

  void ActivateSystemProxyService(net::ProxyConfigService* system_proxy_service);

  void SetProxyConfig(const std::string& rules);

  // net::ProxyConfigService methods
  virtual void AddObserver(net::ProxyConfigService::Observer* observer) OVERRIDE;
  virtual void RemoveObserver(net::ProxyConfigService::Observer* observer) OVERRIDE;
  virtual net::ProxyConfigService::ConfigAvailability
      GetLatestProxyConfig(net::ProxyConfig* config) OVERRIDE;

  // net::ProxyConfigService::Observer method
  virtual void OnProxyConfigChanged(const net::ProxyConfig& config,
      net::ProxyConfigService::ConfigAvailability availability) OVERRIDE;

 private:
  net::ProxyConfigService::ConfigAvailability status_;
  bool is_manual_;
  bool is_in_init_;
  ObserverList<net::ProxyConfigService::Observer> observers_;
  scoped_ptr<net::ProxyConfig> config_;
  scoped_ptr<net::ProxyConfigService> system_proxy_service_;

  base::ThreadChecker thread_checker_;
};

} // namespace cnet

#endif  // YAHOO_CNET_CNET_PROXY_SERVICE_H_
