/*
 * rp-schedulable-cb-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include <evhtp.h>
#include "event/rp-event-impl-base.h"
#include "rp-schedulable-cb.h"

G_BEGIN_DECLS

/**
 * libevent implementation of SchedulableCallback.
 */
#define RP_TYPE_SCHEDULABLE_CALLBACK_IMPL rp_schedulable_callback_impl_get_type()
G_DECLARE_FINAL_TYPE(RpSchedulableCallbackImpl, rp_schedulable_callback_impl, RP, SCHEDULABLE_CALLBACK_IMPL, RpEventImplBase)

RpSchedulableCallbackImpl* rp_schedulable_callback_impl_new(evbase_t* evbase,
                                                            RpSchedulableCallbackCb cb,
                                                            gpointer arg);

G_END_DECLS
