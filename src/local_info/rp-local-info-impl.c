/*
 * rp-local-info-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_local_info_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_local_info_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include <stdio.h>
#include "local_info/rp-local-info-impl.h"

struct _RpLocalInfoImpl {
    GObject parent_instance;

    struct sockaddr* m_address;
};

static void local_info_iface_init(RpLocalInfoInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpLocalInfoImpl, rp_local_info_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_LOCAL_INFO, local_info_iface_init)
)

static struct sockaddr*
address_i(RpLocalInfo* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_LOCAL_INFO_IMPL(self)->m_address;
}

static void
local_info_iface_init(RpLocalInfoInterface* iface)
{
    LOGD("(%p)", iface);
    iface->address = address_i;
}

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_local_info_impl_parent_class)->dispose(object);
}

static void
rp_local_info_impl_class_init(RpLocalInfoImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_local_info_impl_init(RpLocalInfoImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpLocalInfoImpl*
rp_local_info_impl_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_LOCAL_INFO_IMPL, NULL);
}
