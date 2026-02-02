/*
 * rp-dns-factory-util.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_dns_factory_util_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dns_factory_util_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/dns_resolver/libevent/rp-dns-impl.h"
#include "network/dns_resolver/rp-dns-factory-util.h"

RpDnsResolverFactory*
rp_dns_resolver_factory_create_factory(void/* TODO...typed_dns_resolver_config*/)
{
    LOGD("()");
    RpDnsResolverFactory* factory = RP_DNS_RESOLVER_FACTORY(rp_libevent_dns_resolver_factory_new());
    rp_dns_resolver_factory_initialize(factory);
    return factory;
}

RpDnsResolverFactory*
rp_dns_factory_util_create_dns_resolver_factory_from_typed_config(/* TODO...typed_dns_resolver_config */)
{
    LOGD("()");
    return rp_dns_resolver_factory_create_factory();
}

RpDnsResolverFactory*
rp_dns_factory_util_create_dns_resolver_factory_from_proto(rproxy_t* config G_GNUC_UNUSED/*, TODO...typed_dns_resolver_config*/)
{
    LOGD("(%p)", config);
    g_return_val_if_fail(config != NULL, NULL);
    //TODO...typed_dns_resolver_config = makeDnsReesolverConfig(config);
    return rp_dns_factory_util_create_dns_resolver_factory_from_typed_config();
}
