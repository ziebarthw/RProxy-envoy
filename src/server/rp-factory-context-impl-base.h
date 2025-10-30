/*
 * rp-factory-context-impl-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-factory-context.h"

G_BEGIN_DECLS

#define RP_TYPE_FACTORY_CONTEXT_IMPL_BASE rp_factory_context_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpFactoryContextImplBase, rp_factory_context_impl_base, RP, FACTORY_CONTEXT_IMPL_BASE, GObject)

struct _RpFactoryContextImplBaseClass {
    GObjectClass parent_class;

};

G_END_DECLS
