/*
 * rp-server-instance-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_instance_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_instance_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "server/rp-server-instance-impl.h"

struct _RpServerInstanceImpl {
    RpServerInstanceBase parent_instance;
};

G_DEFINE_FINAL_TYPE(RpServerInstanceImpl, rp_server_instance_impl, RP_TYPE_SERVER_INSTANCE_BASE)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_server_instance_impl_parent_class)->dispose(obj);
}

static void
rp_server_instance_impl_class_init(RpServerInstanceImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_server_instance_impl_init(RpServerInstanceImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpServerInstance*
rp_server_instance_impl_new(rproxy_t* bootstrap, RpThreadLocalInstance* tls, evthr_t* thr)
{
    LOGD("(%p, %p, %p)", bootstrap, tls, thr);
    g_return_val_if_fail(bootstrap != NULL, NULL);
    g_return_val_if_fail(RP_IS_THREAD_LOCAL_INSTANCE(tls), NULL);
    return RP_SERVER_INSTANCE(g_object_new(RP_TYPE_SERVER_INSTANCE_IMPL,
                                            "bootstrap", bootstrap,
                                            "tls", tls,
                                            "thr", thr,
                                            NULL));
}
