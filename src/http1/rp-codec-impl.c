/*
 * rp-codec-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-codec-impl.h"

#if 0
// https://github.com/envoyproxy/envoy/blob/main/source/extensions/filters/network/http_connection_manager/config.cc
//     Http::ServerConnectionPtr HttpConnectionManagerConfig::createCodec(...)
// https://github.com/envoyproxy/envoy/blob/main/source/common/http/conn_manager_impl.cc
//     ConnectionManagerImpl::createCodec(....) { config_.createCodec(...) }
#include "rp-server-connection-impl.h"
void t(void)
{
    evhtp_connection_t* connection_;
    RpServerConnectionCallbacks* callbacks_;
    struct RpHttp1Settings_s settings_;
    rp_server_connection_impl_new(connection_, callbacks_, &settings_, 1024, 1024);
}
#endif//0
