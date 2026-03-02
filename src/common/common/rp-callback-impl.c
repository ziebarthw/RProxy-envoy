/*
 * rp-callback-impl.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_callback_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_callback_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "common/common/rp-callback-impl.h"

struct _RpCallbackManager {
    GObject parent_instance;

    GList* /* <CallbackHolder> */ m_callbacks;
};

G_DEFINE_TYPE(RpCallbackManager, rp_callback_manager, G_TYPE_OBJECT)

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpCallbackManager* self = RP_CALLBACK_MANAGER(obj);
    g_list_free_full(g_steal_pointer(&self->m_callbacks), g_object_unref);

    G_OBJECT_CLASS(rp_callback_manager_parent_class)->dispose(obj);
}

static void
rp_callback_manager_class_init(RpCallbackManagerClass* klass)
{
    LOGD("(%p)", klass);
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_callback_manager_init(RpCallbackManager* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_callbacks = NULL;
}

RpCallbackManager*
rp_callback_manager_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_CALLBACK_MANAGER, NULL);
}

RpCallbackHandlePtr
rp_callback_manager_add(RpCallbackManager* self, RpCallback cb, gpointer arg)
{
    LOGD("(%p, %p, %p)", self, cb, arg);
    g_return_val_if_fail(RP_IS_CALLBACK_MANAGER(self), NULL);
    g_return_val_if_fail(cb != NULL, NULL);
    RpCallbackHolder* handle = rp_callback_holder_new(self, cb, arg);
    self->m_callbacks = g_list_append(self->m_callbacks, handle);
    return RP_CALLBACK_HANDLE(handle);
}

GList*
rp_callback_manager_callbacks(RpCallbackManager* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_CALLBACK_MANAGER(self), NULL);
    return self->m_callbacks;
}
