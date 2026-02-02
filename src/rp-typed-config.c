/*
 * rp-typed-config.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include "macrologger.h"

#if (defined(rp_typed_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_typed_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-typed-config.h"

G_DEFINE_INTERFACE(RpUntypedFactory, rp_untyped_factory, G_TYPE_OBJECT)
static void
rp_untyped_factory_default_init(RpUntypedFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}

G_DEFINE_INTERFACE(RpTypedFactory, rp_typed_factory, RP_TYPE_UNTYPED_FACTORY)
static void
rp_typed_factory_default_init(RpTypedFactoryInterface* iface G_GNUC_UNUSED)
{
    LOGD("(%p)", iface);
}
