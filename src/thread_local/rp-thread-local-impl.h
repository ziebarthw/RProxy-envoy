/*
 * rp-thread-local-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-thread-local.h"

G_BEGIN_DECLS

typedef struct _RpThreadLocalData RpThreadLocalData;
struct _RpThreadLocalData {
    SHARED_PTR(RpDispatcher) m_dispatcher;
    UNIQUE_PTR(GPtrArray) m_data;
};

/**
 * Implementation of ThreadLocal that relies on static thread_local objects.
 */
#define RP_TYPE_THREAD_LOCAL_INSTANCE_IMPL rp_thread_local_instance_impl_get_type()
G_DECLARE_FINAL_TYPE(RpThreadLocalInstanceImpl, rp_thread_local_instance_impl, RP, THREAD_LOCAL_INSTANCE_IMPL, GObject)

RpThreadLocalInstanceImpl* rp_thread_local_instance_impl_new(void);
void rp_thread_local_instance_impl_run_on_all_threads(RpThreadLocalInstanceImpl* self,
                                                        void (*cb)(gpointer),
                                                        gpointer arg);
void rp_thread_local_instance_impl_run_on_all_threads_completed(RpThreadLocalInstanceImpl* self,
                                                                void (*cb)(gpointer),
                                                                gpointer cb_arg,
                                                                void (*main_callback)(gpointer),
                                                                gpointer main_callback_arg,
                                                                GDestroyNotify destroy_cb);
void rp_thread_local_instance_impl_remove_slot(RpThreadLocalInstanceImpl* self,
                                                guint32 index);
RpDispatcher* rp_thread_local_instance_impl_get_dispatcher(void);
GList* rp_thread_local_instance_impl_registered_threads_(RpThreadLocalInstanceImpl* self);
RpDispatcher* rp_thread_local_instance_impl_dispatcher_(RpThreadLocalInstanceImpl* self);
RpDispatcher* rp_thread_local_instance_impl_main_thread_dispatcher_(RpThreadLocalInstanceImpl* self);
bool rp_thread_local_instance_impl_shutdown_(RpThreadLocalInstanceImpl* self);

G_END_DECLS
