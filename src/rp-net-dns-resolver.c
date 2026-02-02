/*
 * rp-net-dns-resolver.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_net_dns_resolver_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_net_dns_resolver_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-net-dns-resolver.h"

const char* RpLibeventDnsResolver = "rproxy.network.dns_resolver.libevent";

G_DEFINE_INTERFACE(RpDnsResolverFactory, rp_dns_resolver_factory, RP_TYPE_TYPED_FACTORY)
static void
rp_dns_resolver_factory_default_init(RpDnsResolverFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
