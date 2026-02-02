/*
 * rp-net-dns-resolver.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-dns.h"

G_BEGIN_DECLS

extern const char* RpLibeventDnsResolver;


/**
 * RpDnsResolverFactory
 */
#define RP_TYPE_DNS_RESOLVER_FACTORY rp_dns_resolver_factory_get_type()
G_DECLARE_INTERFACE(RpDnsResolverFactory, rp_dns_resolver_factory, RP, DNS_RESOLVER_FACTORY, RpTypedFactory)

struct _RpDnsResolverFactoryInterface {
    RpTypedFactoryInterface parent_iface;

    RpNetworkDnsResolverSharedPtr (*create_dns_resolver)(RpDnsResolverFactory*, RpDispatcher*);
    void (*initialize)(RpDnsResolverFactory*);
    void (*terminate)(RpDnsResolverFactory*);
};
static inline RpNetworkDnsResolverSharedPtr
rp_dns_resolver_factory_create_dns_resolver(RpDnsResolverFactory* self, RpDispatcher* dispatcher)
{
    return RP_IS_DNS_RESOLVER_FACTORY(self) ?
        RP_DNS_RESOLVER_FACTORY_GET_IFACE(self)->create_dns_resolver(self, dispatcher) :
        NULL;
}
static inline void
rp_dns_resolver_factory_initialize(RpDnsResolverFactory* self)
{
    if (RP_IS_DNS_RESOLVER_FACTORY(self)) \
        RP_DNS_RESOLVER_FACTORY_GET_IFACE(self)->initialize(self);
}
static inline void
rp_dns_resolver_factory_terminate(RpDnsResolverFactory* self)
{
    if (RP_IS_DNS_RESOLVER_FACTORY(self)) \
        RP_DNS_RESOLVER_FACTORY_GET_IFACE(self)->terminate(self);
}


/**
 * Create the DNS resolver factory based on the typed config and initialize it.
 * @returns the DNS Resolver factory.
 * @param typed_dns_resolver_config: the typed DNS resolver config
 */
RpDnsResolverFactory* rp_dns_resolver_factory_create_factory(void/* TODO...typed_dns_resolver_config*/);


G_END_DECLS
