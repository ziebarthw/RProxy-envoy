/*
 * rp-host-description-impl-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-host-description.h"

G_BEGIN_DECLS

/**
 * Base implementation of most of Upstream::HostDescription, shared between
 * HostDescriptionImpl and LogicalHost, which is in
 * source/extensions/clusters/common/logical_host.h. These differ in threading.
 *
 * HostDescriptionImpl and HostImpl are intended to be initialized in the main
 * thread, and are thereafter read-only, and thus do not require locking.
 *
 * LogicalHostImpl is intended to be dynamically changed due to DNS resolution
 * and Happy Eyeballs from multiple threads, and thus requires an address_lock
 * and lock annotations to enforce this.
 *
 * The two level implementation inheritance allows most of the implementation
 * to be shared, but sinks the ones requiring different lock semantics into
 * the leaf subclasses.
 */
#define RP_TYPE_HOST_DESCRIPTION_IMPL_BASE rp_host_description_impl_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpHostDescriptionImplBase, rp_host_description_impl_base, RP, HOST_DESCRIPTION_IMPL_BASE, GObject)

struct _RpHostDescriptionImplBaseClass {
    GObjectClass parent_class;

};

struct sockaddr* rp_host_description_impl_base_dest_address_(RpHostDescriptionImplBase* self);

G_END_DECLS
