/*
 * rp-factory-context-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_factory_context_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_factory_context_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "server/rp-factory-context-impl.h"

struct _RpFactoryContextImpl {
    RpFactoryContextImplBase parent_instance;

};

G_DEFINE_FINAL_TYPE(RpFactoryContextImpl, rp_factory_context_impl, RP_TYPE_FACTORY_CONTEXT_IMPL_BASE)

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_factory_context_impl_parent_class)->dispose(object);
}

static void
rp_factory_context_impl_class_init(RpFactoryContextImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_factory_context_impl_init(RpFactoryContextImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpFactoryContextImpl*
rp_factory_context_impl_new(RpInstance* server)
{
    LOGD("(%p)", server);
    g_return_val_if_fail(RP_IS_INSTANCE(server), NULL);
    return g_object_new(RP_TYPE_FACTORY_CONTEXT_IMPL,
                        "server", server,
                        NULL);
}
