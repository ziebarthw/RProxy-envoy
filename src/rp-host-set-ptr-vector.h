/*
 * rp-host-set-ptr-vector.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-upstream.h"

G_BEGIN_DECLS

typedef struct _RpHostSetPtrVector RpHostSetPtrVector;

/* owning container */
RpHostSetPtrVector* rp_host_set_ptr_vector_new(void);
void rp_host_set_ptr_vector_unref(RpHostSetPtrVector* self);

/* read-only API (“const std::vector<HostSetPtr>”) */
guint rp_host_set_ptr_vector_size(const RpHostSetPtrVector* self);
RpHostSetPtr rp_host_set_ptr_vector_get(const RpHostSetPtrVector* self,
                                        guint idx); /* borrowed */
RpHostSetPtr rp_host_set_ptr_vector_dup(const RpHostSetPtrVector* self,
                                        guint idx); /* owned */

/* owner-only helpers (keep in a “private” header if you want) */
void rp_host_set_ptr_vector_ensure_size(RpHostSetPtrVector* self, guint n);
void rp_host_set_ptr_vector_set(RpHostSetPtrVector* self,
                                guint idx,
                                RpHostSetPtr host_set); /* stores ref */

G_END_DECLS
