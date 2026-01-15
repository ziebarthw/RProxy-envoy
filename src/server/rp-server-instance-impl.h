/*
 * rp-server-instance-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-server-instance.h"
#include "rp-thread-local.h"
#include "server/rp-server-instance-base.h"

G_BEGIN_DECLS

/*
 * The production server instance, which creates all of the required components.
 */
#define RP_TYPE_SERVER_INSTANCE_IMPL rp_server_instance_impl_get_type()
G_DECLARE_FINAL_TYPE(RpServerInstanceImpl, rp_server_instance_impl, RP, SERVER_INSTANCE_IMPL, RpServerInstanceBase)

RpServerInstance* rp_server_instance_impl_new(rproxy_t* bootstrap,
                                                RpThreadLocalInstance* tls,
                                                evthr_t* thr);

G_END_DECLS
