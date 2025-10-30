/*
 * rp-managed-resource-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_managed_resource_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_managed_resource_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include <stdio.h>
#include "upstream/rp-managed-resource-impl.h"

struct _RpManagedResourceImpl {
    RpBasicResourceLimitImpl parent_instance;

};

G_DEFINE_FINAL_TYPE(RpManagedResourceImpl, rp_managed_resource_impl, RP_TYPE_BASIC_RESOURCE_LIMIT_IMPL)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_managed_resource_impl_parent_class)->dispose(obj);
}

static void
rp_managed_resource_impl_class_init(RpManagedResourceImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_managed_resource_impl_init(RpManagedResourceImpl* self)
{
    NOISY_MSG_("(%p)", self);
}

RpManagedResourceImpl*
rp_manage_resource_impl_new(guint64 max, const char* runtime_key)
{
    LOGD("(%zu, %p(%s))", max, runtime_key, runtime_key);
    return g_object_new(RP_TYPE_MANAGED_RESOURCE_IMPL,
                        "max", max,
                        "runtime-key", runtime_key,
                        NULL);
}
