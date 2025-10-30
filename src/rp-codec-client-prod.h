/*
 * rp-codec-client-prod.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-connection.h"
#include "rp-host-description.h"
#include "rp-codec-client.h"

G_BEGIN_DECLS

/**
 * Production implementation that installs a real codec.
 */
#define RP_TYPE_CODEC_CLIENT_PROD rp_codec_client_prod_get_type()
G_DECLARE_FINAL_TYPE(RpCodecClientProd, rp_codec_client_prod, RP, CODEC_CLIENT_PROD, RpCodecClient)

RpCodecClientProd* rp_codec_client_prod_new(RpCodecType_e type,
                                            RpNetworkClientConnection* connection,
                                            RpHostDescription* host,
                                            RpDispatcher* dispatcher,
                                            bool should_connect_on_creation);

G_END_DECLS
