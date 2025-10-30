/*
 * rp-instance-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_instance_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_instance_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "server/rp-instance-impl.h"

struct _RpInstanceImpl {
    RpInstanceBase parent_instance;
};

G_DEFINE_FINAL_TYPE(RpInstanceImpl, rp_instance_impl, RP_TYPE_INSTANCE_BASE)

OVERRIDE void
dispose(GObject* object)
{
    LOGD("(%p)", object);
    G_OBJECT_CLASS(rp_instance_impl_parent_class)->dispose(object);
}

static void
rp_instance_impl_class_init(RpInstanceImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_instance_impl_init(RpInstanceImpl* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpInstanceImpl*
rp_instance_impl_new(const rproxy_hooks_t* hooks)
{
    LOGD("()");
    g_return_val_if_fail(hooks != NULL, NULL);
    return g_object_new(RP_TYPE_INSTANCE_IMPL,
                        "hooks", hooks,
                        NULL);
}
