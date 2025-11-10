/*
 * rp-filter-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-filter-manager.h"
#include "rp-filter-factory.h"

G_DEFINE_INTERFACE(RpFilterChainManager, rp_filter_chain_manager, G_TYPE_OBJECT)

static void
apply_filter_factory_cb_(RpFilterChainManager* self G_GNUC_UNUSED, const struct RpFilterContext* context G_GNUC_UNUSED, RpFilterFactoryCb* factory G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
}

static void
rp_filter_chain_manager_default_init(RpFilterChainManagerInterface* iface)
{
    LOGD("(%p)", iface);
    iface->apply_filter_factory_cb = apply_filter_factory_cb_;
}

void
rp_filter_chain_manager_apply_filter_factory_cb(RpFilterChainManager* self, const struct RpFilterContext* context, RpFilterFactoryCb* factory)
{
    LOGD("(%p, %p, %p(%p))", self, context, factory, factory->m_free_func);
    g_return_if_fail(RP_IS_FILTER_MANAGER(self));
    g_return_if_fail(factory != NULL);
    RP_FILTER_CHAIN_MANAGER_GET_IFACE(self)->apply_filter_factory_cb(self, context, factory);
}

G_DEFINE_INTERFACE(RpFilterChainFactory, rp_filter_chain_factory, G_TYPE_OBJECT)

static bool
create_filter_chain_(RpFilterChainFactory* self G_GNUC_UNUSED, RpFilterChainManager* manager G_GNUC_UNUSED)
{
    g_debug("not implemented at line %d", __LINE__);
    return false;
}

static void
rp_filter_chain_factory_default_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_;
}

bool
rp_filter_chain_factory_create_filter_chain(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    LOGD("(%p, %p)", self, manager);
    g_return_val_if_fail(RP_IS_FILTER_CHAIN_FACTORY(self), false);
    g_return_val_if_fail(RP_IS_FILTER_CHAIN_MANAGER(manager), false);
    return RP_FILTER_CHAIN_FACTORY_GET_IFACE(self)->create_filter_chain(self, manager);
}
