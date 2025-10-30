/*
 * rp-default-upstream-http-filter-chain-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_default_upstream_http_filter_chain_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_default_upstream_http_filter_chain_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "router/rp-default-upstream-http-filter-chain-factory.h"

typedef struct _FilterFactoryProvider FilterFactoryProvider;
typedef struct _FilterFactoryProvider * FilterFactoryProviderPtr;
struct _FilterFactoryProvider {
    RpFilterFactoryCb* m_provider;
    bool m_disabled;
};

static inline FilterFactoryProvider
filter_factory_provider_ctor(RpFilterFactoryCb* provider, bool disabled)
{
    FilterFactoryProvider self = {
        .m_provider = provider,
        .m_disabled = disabled
    };
    return self;
}

static inline FilterFactoryProviderPtr
filter_factory_provider_new(RpFilterFactoryCb* provider, bool disabled)
{
    NOISY_MSG_("(%p(%p), %u)", provider, provider->m_free_func, disabled);
    FilterFactoryProviderPtr self = g_new0(FilterFactoryProvider, 1);
    *self = filter_factory_provider_ctor(provider, disabled);
    return self;
}

static void
filter_factory_provider_free(FilterFactoryProviderPtr self)
{
    NOISY_MSG_("(%p)", self);
    RpFilterFactoryCb* provider = self->m_provider;
    provider->m_free_func(provider);
    g_free(self);
}

struct _RpDefaultUpstreamHttpFilterChainFactory {
    GObject parent_instance;

    GSList* m_filter_factories;
};

static void filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpDefaultUpstreamHttpFilterChainFactory, rp_default_upstream_http_filter_chain_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY, filter_chain_factory_iface_init)
)

static inline void
add_filter_factory(RpDefaultUpstreamHttpFilterChainFactory* self, RpFilterFactoryCb* cb, bool disabled)
{
    NOISY_MSG_("(%p, %p, %u)", self, cb, disabled);
    self->m_filter_factories = g_slist_append(self->m_filter_factories,
                                                filter_factory_provider_new(cb, disabled));
}

static bool
create_filter_chain_i(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    NOISY_MSG_("(%p, %p)", self, manager);

    RpDefaultUpstreamHttpFilterChainFactory* me = RP_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY(self);
    for (GSList* itr = me->m_filter_factories; itr; itr = itr->next)
    {
        FilterFactoryProviderPtr factory = itr->data;
        if (factory->m_disabled)
        {
            NOISY_MSG_("disabled");
            continue;
        }

        rp_filter_chain_manager_apply_filter_factory_cb(manager, NULL, factory->m_provider);
    }

    return true;
}

static void
filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_i;
}

static inline void
filter_factory_free(RpFilterFactoryCb* cb)
{
    NOISY_MSG_("(%p(%p))", cb, cb->m_free_func);
    cb->m_free_func(cb);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDefaultUpstreamHttpFilterChainFactory* self = RP_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY(obj);
    g_slist_free_full(g_steal_pointer(&self->m_filter_factories), (GDestroyNotify)filter_factory_provider_free);

    G_OBJECT_CLASS(rp_default_upstream_http_filter_chain_factory_parent_class)->dispose(obj);
}

static void
rp_default_upstream_http_filter_chain_factory_class_init(RpDefaultUpstreamHttpFilterChainFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_default_upstream_http_filter_chain_factory_init(RpDefaultUpstreamHttpFilterChainFactory* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_filter_factories = NULL;
}

static inline gpointer
default_upstream_http_filter_chain_factory_new(void* arg G_GNUC_UNUSED)
{
    NOISY_MSG_("()");
    return g_object_new(RP_TYPE_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY, NULL);
}

RpDefaultUpstreamHttpFilterChainFactory*
rp_default_upstream_http_filter_chain_factory_new(void)
{
    static GOnce my_once = G_ONCE_INIT;
    LOGD("()");
    g_once(&my_once, default_upstream_http_filter_chain_factory_new, NULL);
    return RP_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY(my_once.retval);
}

void
rp_default_upstream_http_filter_chain_factory_add_filter_factory(RpDefaultUpstreamHttpFilterChainFactory* self, RpFilterFactoryCb* cb, bool disabled)
{
    LOGD("(%p, %p(%p), %u)", self, cb, cb->m_free_func, disabled);
    g_return_if_fail(RP_IS_DEFAULT_UPSTREAM_HTTP_FILTER_CHAIN_FACTORY(self));
    g_return_if_fail(cb != NULL);
    add_filter_factory(self, cb, disabled);
}
