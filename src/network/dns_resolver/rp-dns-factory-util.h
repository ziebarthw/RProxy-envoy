/*
 * rp-dns-factory-util.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-net-dns-resolver.h"

G_BEGIN_DECLS

/**
 * Create the DNS resolver factory from the typed config. This is the underline
 * function which performs the registry lookup based on typed config.
 */
RpDnsResolverFactory* rp_dns_factory_util_create_dns_resolver_factory_from_typed_config(/* TODO...typed_dns_resolver_config */);

/**
 * Create the DNS resolver factory from the proto config.
 * The passed in config parameter may contain invalid typed_dns_resolver_config.
 * In that case, the underline registry lookup will throw an exception.
 * This function has to be called in main thread.
 */
RpDnsResolverFactory* rp_dns_factory_util_create_dns_resolver_factory_from_proto(rproxy_t* bootstrap/*, TODO...typed_dns_resolver_config*/);

G_END_DECLS
