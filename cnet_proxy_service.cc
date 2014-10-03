// Copyright 2014, Yahoo! Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "yahoo/cnet/cnet_proxy_service.h"

namespace cnet {

ProxyConfigService::ProxyConfigService()
    : is_manual_(false), is_in_init_(true) {
  status_ = ProxyConfigService::CONFIG_PENDING;
  config_.reset(new net::ProxyConfig);
  *config_ = net::ProxyConfig::CreateDirect();
}

ProxyConfigService::~ProxyConfigService() {
  if (system_proxy_service_ != NULL) {
    system_proxy_service_->RemoveObserver(this);
  }
}

void ProxyConfigService::ActivateSystemProxyService(
    net::ProxyConfigService *system_proxy_service) {
  DCHECK(thread_checker_.CalledOnValidThread());

  is_in_init_ = false;
  system_proxy_service_.reset(system_proxy_service);
  
  if (system_proxy_service_ != NULL) {
    // Start observing.
    system_proxy_service_->AddObserver(this);
  } else {
    LOG(WARNING) << "Running without system proxy support.";
  }

  if (!is_manual_) {
    net::ProxyConfig config;
    net::ProxyConfigService::ConfigAvailability status;
    if (system_proxy_service_ != NULL) {
    // Get a config that happened before we started observing.
      status = system_proxy_service_->GetLatestProxyConfig(&config);
    } else {
      config = net::ProxyConfig::CreateDirect();
      status = ProxyConfigService::CONFIG_VALID;
    }
    OnProxyConfigChanged(config, status);
  }
}

void ProxyConfigService::OnProxyConfigChanged(
    const net::ProxyConfig& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!is_manual_) {
    status_ = availability;
    config_.reset(new net::ProxyConfig());
    *config_ = config;

    if (status_ != ProxyConfigService::CONFIG_PENDING) {
      // Let the observers know that something changed.
      FOR_EACH_OBSERVER(net::ProxyConfigService::Observer, observers_,
          OnProxyConfigChanged(*config_, status_));
    }
  }
}

void ProxyConfigService::AddObserver(
    net::ProxyConfigService::Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.AddObserver(observer);
}

void ProxyConfigService::RemoveObserver(
    net::ProxyConfigService::Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

void ProxyConfigService::SetProxyConfig(const std::string& rules) {
  DCHECK(thread_checker_.CalledOnValidThread());

  config_.reset(new net::ProxyConfig());
  if (!rules.empty()) {
    // Use a manually-specified config.
    is_manual_ = true;
    config_->proxy_rules().ParseFromString(rules);
    status_ = ProxyConfigService::CONFIG_VALID;
  } else if (system_proxy_service_ != NULL) {
    is_manual_ = false;
    // Use the system's configuration.
    status_ = system_proxy_service_->GetLatestProxyConfig(config_.get());
  } else if (is_in_init_) {
    // We are still waiting for the system's configuration.
    *config_ = net::ProxyConfig::CreateDirect();
    if (status_ != ProxyConfigService::CONFIG_PENDING) {
      // We can't return to the CONFIG_PENDING state, so we assume no proxy.
      is_manual_ = false;
      *config_ = net::ProxyConfig::CreateDirect();
      status_ = ProxyConfigService::CONFIG_VALID;
    }
  } else {
    // Disable the proxy.
    is_manual_ = false;
    *config_ = net::ProxyConfig::CreateDirect();
    status_ = ProxyConfigService::CONFIG_VALID;
  }

  // Let the observers know that something changed.
  if (status_ != ProxyConfigService::CONFIG_PENDING) {
    FOR_EACH_OBSERVER(net::ProxyConfigService::Observer, observers_,
        OnProxyConfigChanged(*config_, status_));
  }
}

net::ProxyConfigService::ConfigAvailability
ProxyConfigService::GetLatestProxyConfig(net::ProxyConfig* config) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (is_manual_ || (system_proxy_service_ == NULL)) {
    *config = *config_;
    return status_;
  } else {
    return system_proxy_service_->GetLatestProxyConfig(config);
  }
}

} // namespace cnet
