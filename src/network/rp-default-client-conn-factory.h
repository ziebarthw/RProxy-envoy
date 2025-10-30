/*
 * rp-default-client-conn-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-client-conn-factory.h"

G_BEGIN_DECLS

/**
 * This client connection factory handles the connection if the remote address type is either ip or
 * pipe.
 */
#define RP_TYPE_DEFAULT_CLIENT_CONNECTION_FACTORY rp_default_client_connection_factory_get_type()
G_DECLARE_FINAL_TYPE(RpDefaultClientConnectionFactory, rp_default_client_connection_factory, RP, DEFAULT_CLIENT_CONNECTION_FACTORY, GObject)

RpDefaultClientConnectionFactory* rp_default_client_connection_factory_new(void);

G_END_DECLS
