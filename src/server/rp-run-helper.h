/*
 * rp-run-helper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-cluster-manager.h"
#include "rp-dispatcher.h"
#include "rp-server-instance.h"

G_BEGIN_DECLS

/**
 * This is a helper used by InstanceBase::run() on the stack. It's broken out to make testing
 * easier.
 */
#define RP_TYPE_RUN_HELPER rp_run_helper_get_type()
G_DECLARE_FINAL_TYPE(RpRunHelper, rp_run_helper, RP, RUN_HELPER, GObject)

RpRunHelper* rp_run_helper_new(RpServerInstance* instance,
                                /*TODO...const Options& options,*/
                                RpDispatcher* dispatcher,
                                RpClusterManager* cm/*TODO...,
                                void(*workers_start_cb)(gpointer), gpointer workers_start_arg*/);

G_END_DECLS
