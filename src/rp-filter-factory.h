/*
 * rp-filter-factory.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _RpFilterChainFactoryCallbacks RpFilterChainFactoryCallbacks;

/**
 * This function is used to wrap the creation of an HTTP filter chain for new streams as they
 * come in. Filter factories create the function at configuration initialization time, and then
 * they are used at runtime.
 * @param callbacks supplies the callbacks for the stream to install filters to. Typically the
 * function will install a single filter, but it's technically possibly to install more than one
 * if desired.
 */
typedef struct _RpFilterFactoryCb RpFilterFactoryCb;
struct _RpFilterFactoryCb {
    void (*m_cb)(RpFilterFactoryCb*, RpFilterChainFactoryCallbacks*);
    GDestroyNotify m_free_func;
};
#define RP_FILTER_FACTORY_CB(s) (RpFilterFactoryCb*)(s)
static inline RpFilterFactoryCb
rp_filter_factory_cb_ctor(void (*cb)(RpFilterFactoryCb*, RpFilterChainFactoryCallbacks*), GDestroyNotify free_func)
{
    RpFilterFactoryCb self = {
        .m_cb = cb,
        .m_free_func = free_func
    };
    return self;
}
static inline RpFilterFactoryCb*
rp_filter_factory_cb_new(void (*cb)(RpFilterFactoryCb*, RpFilterChainFactoryCallbacks*), GDestroyNotify free_func)
{
    RpFilterFactoryCb* self = g_new0(RpFilterFactoryCb, 1);
    *self = rp_filter_factory_cb_ctor(cb, free_func);
    return self;
}


/**
 * Simple struct of additional contextual information of HTTP filter, e.g. filter config name
 * from configuration, etc.
 */
struct RpFilterContext {
    // The name of the filter configuration that used to create related filter factory function.
    // This could be any legitimate non-empty string.
    const char* config_name;
};

/**
 * The filter chain manager is provided by the connection manager to the filter chain factory.
 * The filter chain factory will post the filter factory context and filter factory to the
 * filter chain manager to create filter and construct HTTP stream filter chain.
 */
#define RP_TYPE_FILTER_CHAIN_MANAGER rp_filter_chain_manager_get_type()
G_DECLARE_INTERFACE(RpFilterChainManager, rp_filter_chain_manager, RP, FILTER_CHAIN_MANAGER, GObject)

struct _RpFilterChainManagerInterface {
    GTypeInterface parent_iface;

    /**
     * Post filter factory context and filter factory to the filter chain manager. The filter
     * chain manager will create filter instance based on the context and factory internally.
     * @param context supplies additional contextual information of filter factory.
     * @param factory factory function used to create filter instances.
     */
    void (*apply_filter_factory_cb)(RpFilterChainManager*,
                                    const struct RpFilterContext*,
                                    RpFilterFactoryCb*);
};

void rp_filter_chain_manager_apply_filter_factory_cb(RpFilterChainManager* self,
                                                const struct RpFilterContext* context,
                                                RpFilterFactoryCb* factory);

/**
 * A FilterChainFactory is used by a connection manager to create an HTTP level filter chain when a
 * new stream is created on the connection (either locally or remotely). Typically it would be
 * implemented by a configuration engine that would install a set of filters that are able to
 * process an application scenario on top of a stream.
 */
#define RP_TYPE_FILTER_CHAIN_FACTORY rp_filter_chain_factory_get_type()
G_DECLARE_INTERFACE(RpFilterChainFactory, rp_filter_chain_factory, RP, FILTER_CHAIN_FACTORY, GObject)

struct _RpFilterChainFactoryInterface {
    GTypeInterface parent_iface;

    /**
     * Called when a new HTTP stream is created on the connection.
     * @param manager supplies the "sink" that is used for actually creating the filter chain. @see
     *                FilterChainManager.
     * @param options additional options for creating a filter chain.
     * @return whather a filter chain has been created.
     */
    bool (*create_filter_chain)(RpFilterChainFactory*, RpFilterChainManager*);
};

bool rp_filter_chain_factory_create_filter_chain(RpFilterChainFactory* self,
                                                RpFilterChainManager* manager);

G_END_DECLS
