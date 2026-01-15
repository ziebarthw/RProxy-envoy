/*
 * rp-factory-context-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "server/rp-factory-context-impl-base.h"
#include "rp-server-instance.h"

G_BEGIN_DECLS

/**
 * Implementation of FactoryContext wrapping a Server::Instance and some listener components.
 */
#define RP_TYPE_FACTORY_CONTEXT_IMPL rp_factory_context_impl_get_type()
G_DECLARE_FINAL_TYPE(RpFactoryContextImpl, rp_factory_context_impl, RP, FACTORY_CONTEXT_IMPL, RpFactoryContextImplBase)

RpFactoryContextImpl* rp_factory_context_impl_new(RpServerInstance* server);

G_END_DECLS
