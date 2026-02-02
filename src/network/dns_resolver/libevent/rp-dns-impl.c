/*
 * rp-dns-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dns_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dns_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-address-impl.h"
#include "network/dns_resolver/libevent/rp-dns-impl.h"

struct _RpDnsResolverImpl {
    GObject parent_instance;

    RpDispatcher* m_dispatcher;
    evdns_base_t* m_dns_base;

    GHashTable* m_inflight;

    bool m_dirty_channel : 1;
    bool m_filter_unroutable_families : 1;
};

static void network_dns_resolver_iface_init(RpNetworkDnsResolverInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpDnsResolverImpl, rp_dns_resolver_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_NETWORK_DNS_RESOLVER, network_dns_resolver_iface_init)
)

// Dispatcher post callback that runs in main thread (dns thread).
static void
start_resolution_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpAddrInfoPendingResolution* self = arg;
    RpDnsResolverImpl* me = rp_pending_resolution_parent_(RP_PENDING_RESOLUTION(self));
    rp_addr_info_pending_resolution_start_resolution(self, me->m_inflight);
}

static void
reset_networking_i(RpNetworkDnsResolver* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static RpNetworkActiveDnsQuery*
resolve_i(RpNetworkDnsResolver* self, RpDispatcher* dispatcher, const char* dns_name, RpDnsLookupFamily_e dns_lookup_family,
            RpNetworkDnsResolveCb cb, gpointer arg)
{
    NOISY_MSG_("(%p, %p, %p(%s), %d, %p, %p)",
        self, dispatcher, dns_name, dns_name, dns_lookup_family, cb, arg);

    RpDnsResolverImpl* me = RP_DNS_RESOLVER_IMPL(self);

    if (me->m_dirty_channel)
    {
        LOGD("dirty channel");
    }

    RpAddrInfoPendingResolution* pending_resolution =
        rp_addr_info_pending_resolution_new(me, cb, arg, dispatcher, me->m_dns_base, dns_name, dns_lookup_family);
    rp_dispatcher_base_post(RP_DISPATCHER_BASE(me->m_dispatcher), start_resolution_cb, pending_resolution);
    return RP_NETWORK_ACTIVE_DNS_QUERY(pending_resolution);

#if 0
    rp_addr_info_pending_resolution_start_resolution(pending_resolution);
    if (rp_pending_resolution_completed_(RP_PENDING_RESOLUTION(pending_resolution)))
    {
        NOISY_MSG_("already completed");
        return NULL;
    }
    else
    {
        //TODO...updateTimer();

        rp_pending_resolution_set_owned(RP_PENDING_RESOLUTION(pending_resolution), true);
        g_object_unref(pending_resolution);//pending_resolution.release();????
        return RP_NETWORK_ACTIVE_DNS_QUERY(pending_resolution);
    }
#endif//0
}

static void
network_dns_resolver_iface_init(RpNetworkDnsResolverInterface* iface)
{
    LOGD("(%p)", iface);
    iface->reset_networking = reset_networking_i;
    iface->resolve = resolve_i;
}

static void
dns_base_free(gpointer arg)
{
    evdns_base_free(arg, false/*REVISIT?*/);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpDnsResolverImpl* self = RP_DNS_RESOLVER_IMPL(obj);
    g_clear_pointer(&self->m_dns_base, dns_base_free);
    g_hash_table_destroy(g_steal_pointer(&self->m_inflight));

    G_OBJECT_CLASS(rp_dns_resolver_impl_parent_class)->dispose(obj);
}

static void
rp_dns_resolver_impl_class_init(RpDnsResolverImplClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_dns_resolver_impl_init(RpDnsResolverImpl* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_filter_unroutable_families = true;//REVISIT: should come from config.
    self->m_inflight = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
}

RpDnsResolverImpl*
rp_dns_resolver_impl_new(RpDispatcher* dispatcher)
{
    LOGD("(%p)", dispatcher);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    RpDnsResolverImpl* self = g_object_new(RP_TYPE_DNS_RESOLVER_IMPL, NULL);
    self->m_dispatcher = dispatcher;
    self->m_dns_base = evdns_base_new(evthr_get_base(rp_dispatcher_thr(dispatcher)), 1);
    g_assert(self->m_dns_base);
    return self;
}

void
rp_dns_resolver_impl_set_dirty_channel_(RpDnsResolverImpl* self, bool dirty_channel)
{
    LOGD("(%p, %u)", self, dirty_channel);
    g_return_if_fail(RP_IS_DNS_RESOLVER_IMPL(self));
    self->m_dirty_channel = dirty_channel;
}

bool
rp_dns_resolver_impl_filter_unroutable_families_(RpDnsResolverImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_DNS_RESOLVER_IMPL(self), false);
    return self->m_filter_unroutable_families;
}

GHashTable*
rp_dns_resolver_impl_inflight_(RpDnsResolverImpl* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_DNS_RESOLVER_IMPL(self), NULL);
    return self->m_inflight;
}
