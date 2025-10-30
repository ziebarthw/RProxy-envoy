/*
 * rp-codec-helper.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-codec.h"

G_BEGIN_DECLS

#define RP_TYPE_STREAM_CALLBACK_HELPER rp_stream_callback_helper_get_type()
G_DECLARE_DERIVABLE_TYPE(RpStreamCallbackHelper, rp_stream_callback_helper, RP, STREAM_CALLBACK_HELPER, GObject)

struct _RpStreamCallbackHelperClass {
    GObjectClass parent_class;

};

void rp_stream_callback_helper_add_callbacks_helper(RpStreamCallbackHelper* self,
                                                    RpStreamCallbacks* callbacks);
void rp_stream_callback_helper_remove_callbacks_helper(RpStreamCallbackHelper* self,
                                                        RpStreamCallbacks* callbacks);
void rp_stream_callback_helper_run_low_watermark_callbacks(RpStreamCallbackHelper* self);
void rp_stream_callback_helper_run_high_watermark_callbacks(RpStreamCallbackHelper* self);
void rp_stream_callback_helper_run_reset_callbacks(RpStreamCallbackHelper* self,
                                                    RpStreamResetReason_e reason,
                                                    const char* details);

G_END_DECLS
