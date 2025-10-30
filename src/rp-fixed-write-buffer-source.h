/*
 * rp-fixed-write-buffer-source.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-net-filter-mgr-impl.h"

G_BEGIN_DECLS

/**
 * Adapter that masquerades a given buffer instance as a WriteBufferSource.
 */
#define RP_TYPE_FIXED_WRITE_BUFFER_SOURCE rp_fixed_write_buffer_source_get_type()
G_DECLARE_FINAL_TYPE(RpFixedWriteBufferSource, rp_fixed_write_buffer_source, RP, FIXED_WRITE_BUFFER_SOURCE, GObject)

RpFixedWriteBufferSource* rp_fixed_write_buffer_source_new(evbuf_t* data,
                                                            bool end_stream);

G_END_DECLS
