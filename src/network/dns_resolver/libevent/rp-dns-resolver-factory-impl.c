/*
 * rp-dns-resolver-factory-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dns_resolver_factory_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dns_resolver_factory_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/dns_resolver/libevent/rp-dns-impl.h"

struct _RpLibeventDnsResolverFactory {
    GObject parent_instance;

};

static void untyped_factory_iface_init(RpUntypedFactoryInterface* iface);
static void dns_resolver_factory_iface_init(RpDnsResolverFactoryInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpLibeventDnsResolverFactory, rp_libevent_dns_resolver_factory, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_UNTYPED_FACTORY, untyped_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_TYPED_FACTORY, NULL)
    G_IMPLEMENT_INTERFACE(RP_TYPE_DNS_RESOLVER_FACTORY, dns_resolver_factory_iface_init)
)

static const char*
name_i(RpUntypedFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return RpLibeventDnsResolver;
}

static void
untyped_factory_iface_init(RpUntypedFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->name = name_i;
}

static RpNetworkDnsResolverSharedPtr
create_dns_resolver_i(RpDnsResolverFactory* self G_GNUC_UNUSED, RpDispatcher* dispatcher)
{
    NOISY_MSG_("(%p, %p)", self, dispatcher);
    return RP_NETWORK_DNS_RESOLVER(rp_dns_resolver_impl_new(dispatcher));
}

static void
initialize_i(RpDnsResolverFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
terminate_i(RpDnsResolverFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static void
dns_resolver_factory_iface_init(RpDnsResolverFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_dns_resolver = create_dns_resolver_i;
    iface->initialize = initialize_i;
    iface->terminate = terminate_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_libevent_dns_resolver_factory_parent_class)->dispose(obj);
}

static void
rp_libevent_dns_resolver_factory_class_init(RpLibeventDnsResolverFactoryClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_libevent_dns_resolver_factory_init(RpLibeventDnsResolverFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpLibeventDnsResolverFactory*
rp_libevent_dns_resolver_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_LIBEVENT_DNS_RESOLVER_FACTORY, NULL);
}
