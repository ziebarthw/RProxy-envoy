/*
 * rp-filter-chain-factory-callbacks-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-filter-factory.h"
#include "rp-filter-manager.h"

G_BEGIN_DECLS

#define RP_TYPE_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL rp_filter_chain_factory_callbacks_impl_get_type()
G_DECLARE_FINAL_TYPE(RpFilterChainFactoryCallbacksImpl, rp_filter_chain_factory_callbacks_impl, RP, FILTER_CHAIN_FACTORY_CALLBACKS_IMPL, GObject)

RpFilterChainFactoryCallbacksImpl* rp_filter_chain_factory_callbacks_impl_new(RpFilterManager* manager,
                                                                                const struct RpFilterContext* context);

G_END_DECLS
