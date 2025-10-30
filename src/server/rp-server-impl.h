/*
 * rp-server-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-instance.h"
#include "rp-factory-context.h"

G_BEGIN_DECLS

/*
 * ServerFactoryContextImpl implements both ServerFactoryContext and
 * TransportSocketFactoryContext for convenience as these two contexts
 * share common member functions and member variables.
 */
#define RP_TYPE_SERVER_FACTORY_CONTEXT_IMPL rp_server_factory_context_impl_get_type()
G_DECLARE_FINAL_TYPE(RpServerFactoryContextImpl, rp_server_factory_context_impl, RP, SERVER_FACTORY_CONTEXT_IMPL, GObject)

RpServerFactoryContextImpl* rp_server_factory_context_impl_new(RpInstance* server);

G_END_DECLS
