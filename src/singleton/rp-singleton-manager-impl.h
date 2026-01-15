/*
 * rp-singleton-manager-impl.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-singleton-manager.h"

G_BEGIN_DECLS

/**
 * Implementation of the singleton manager that checks the registry for name validity. It is
 * assumed the singleton manager is only used on the main thread so it is not thread safe. Asserts
 * verify that.
 */
#define RP_TYPE_SINGLETON_MANAGER_IMPL rp_singleton_manager_impl_get_type()
G_DECLARE_FINAL_TYPE(RpSingletonManagerImpl, rp_singleton_manager_impl, RP, SINGLETON_MANAGER_IMPL, GObject)

RpSingletonManagerImpl* rp_singleton_manager_impl_new(void);

G_END_DECLS
