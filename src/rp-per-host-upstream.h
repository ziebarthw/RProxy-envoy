/*
 * rp-per-host-upstream.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-router.h"

G_BEGIN_DECLS

/**
 * Config registration for the HttpConnPool. @see Router::GenericConnPoolFactory
 */
#define RP_TYPE_PER_HOST_GENERIC_CONN_POOL_FACTORY rp_per_host_generic_conn_pool_factory_get_type()
G_DECLARE_FINAL_TYPE(RpPerHostGenericConnPoolFactory, rp_per_host_generic_conn_pool_factory, RP, PER_HOST_GENERIC_CONN_POOL_FACTORY, GObject)

RpPerHostGenericConnPoolFactory* rp_per_host_generic_conn_pool_factory_new(void);

G_END_DECLS
