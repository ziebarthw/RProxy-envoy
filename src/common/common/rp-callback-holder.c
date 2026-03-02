/*
 * rp-callback-holder.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_callback_holder_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_callback_holder_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "common/common/rp-callback-impl.h"

struct _RpCallbackHolder {
    GObject parent_instance;

    RpCallbackManager* m_parent;
    RpCallback m_cb;
    gpointer m_arg;
};

static void callback_handle_iface_init(RpCallbackHandleInterface* iface);

G_DEFINE_TYPE_WITH_CODE(RpCallbackHolder, rp_callback_holder, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_CALLBACK_HANDLE, callback_handle_iface_init)
)

static gpointer
arg_i(RpCallbackHandle* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CALLBACK_HOLDER(self)->m_arg;
}

static RpCallback
cb_i(RpCallbackHandle* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_CALLBACK_HOLDER(self)->m_cb;
}

static void
callback_handle_iface_init(RpCallbackHandleInterface* iface)
{
    LOGD("(%p)", iface);
    iface->arg = arg_i;
    iface->cb = cb_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_callback_holder_parent_class)->dispose(obj);
}

static void
rp_callback_holder_class_init(RpCallbackHolderClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_callback_holder_init(RpCallbackHolder* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpCallbackHolder*
rp_callback_holder_new(RpCallbackManager* parent, RpCallback cb, gpointer arg)
{
    LOGD("(%p, %p, %p)", parent, cb, arg);

    g_return_val_if_fail(RP_IS_CALLBACK_MANAGER(parent), NULL);
    g_return_val_if_fail(cb != NULL, NULL);

    RpCallbackHolder* self = g_object_new(RP_TYPE_CALLBACK_HOLDER, NULL);
    self->m_parent = parent;
    self->m_cb = cb;
    self->m_arg = arg;
    return self;
}
