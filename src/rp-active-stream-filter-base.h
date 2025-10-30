/*
 * rp-active-stream-filter-base.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-http-filter.h"

G_BEGIN_DECLS

typedef enum {
    RpIterationState_Continue,
    RpIterationState_StopSingleIteration,
    RpIterationState_StopAllBuffer,
    RpIterationState_StopAllWatermark,
} RpIterationState_e;

/**
 * Base class wrapper for both stream encoder and decoder filters.
 *
 * This class is responsible for performing matching and updating match data when a match tree is
 * configured for the associated filter. When not using a match tree, only minimal overhead (i.e.
 * memory overhead of unused fields) should apply.
 */
#define RP_TYPE_ACTIVE_STREAM_FILTER_BASE rp_active_stream_filter_base_get_type()
G_DECLARE_DERIVABLE_TYPE(RpActiveStreamFilterBase, rp_active_stream_filter_base, RP, ACTIVE_STREAM_FILTER_BASE, GObject)

struct _RpActiveStreamFilterBaseClass {
    GObjectClass parent_class;

    bool (*can_continue)(RpActiveStreamFilterBase*);
    evbuf_t* (*create_buffer)(RpActiveStreamFilterBase*);
    evbuf_t* (*buffered_data)(RpActiveStreamFilterBase*);
    bool (*observed_end_stream)(RpActiveStreamFilterBase*);
    void (*do_headers)(RpActiveStreamFilterBase*, bool);
    void (*do_data)(RpActiveStreamFilterBase*, bool);
    void (*do_trailers)(RpActiveStreamFilterBase*);
    bool (*has_trailers)(RpActiveStreamFilterBase*);
};

void rp_active_stream_filter_base_common_continue(RpActiveStreamFilterBase* self);
bool rp_active_stream_filter_base_get_headers_continued(RpActiveStreamFilterBase* self);
void rp_active_stream_filter_base_set_headers_continued(RpActiveStreamFilterBase* self);
static inline void
rp_active_stream_filter_base_do_headers(RpActiveStreamFilterBase* self, bool end_stream)
{
    if (RP_IS_ACTIVE_STREAM_FILTER_BASE(self))
    {
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->do_headers(self, end_stream);
    }
}
static inline void
rp_active_stream_filter_base_do_data(RpActiveStreamFilterBase* self, bool end_stream)
{
    if (RP_IS_ACTIVE_STREAM_FILTER_BASE(self))
    {
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->do_data(self, end_stream);
    }
}
static inline void
rp_active_stream_filter_base_do_trailers(RpActiveStreamFilterBase* self)
{
    if (RP_IS_ACTIVE_STREAM_FILTER_BASE(self))
    {
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->do_trailers(self);
    }
}
static inline bool
rp_active_stream_filter_base_has_trailers(RpActiveStreamFilterBase* self)
{
    return RP_IS_ACTIVE_STREAM_FILTER_BASE(self) ?
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->has_trailers(self) : false;
}
static inline evbuf_t*
rp_active_stream_filter_base_create_buffer(RpActiveStreamFilterBase* self)
{
    return RP_IS_ACTIVE_STREAM_FILTER_BASE(self) ?
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->create_buffer(self) : NULL;
}
static inline evbuf_t*
rp_active_stream_filter_base_buffered_data(RpActiveStreamFilterBase* self)
{
    return RP_IS_ACTIVE_STREAM_FILTER_BASE(self) ?
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->buffered_data(self) : NULL;
}
bool rp_active_stream_filter_base_can_iterate(RpActiveStreamFilterBase* self);
void rp_active_stream_filter_base_common_handle_buffer_data(RpActiveStreamFilterBase* self,
                                                            evbuf_t* provided_data);
bool rp_active_stream_filter_base_common_handle_after_headers_callback(RpActiveStreamFilterBase* self,
                                                RpFilterHeadersStatus_e status,
                                                bool* end_stream);
bool rp_active_stream_filter_base_common_handle_after_data_callback(RpActiveStreamFilterBase* self,
                                                        RpFilterDataStatus_e status,
                                                        evbuf_t* provided_data,
                                                        bool* buffer_was_streaming);
bool rp_active_stream_filter_base_common_handle_after_trailers_callback(RpActiveStreamFilterBase* self,
                                                RpFilterTrailerStatus_e status);
bool rp_active_stream_filter_base_iterate_from_current_filter(RpActiveStreamFilterBase* self);
bool rp_active_stream_filter_base_get_end_stream(RpActiveStreamFilterBase* self);
void rp_active_stream_filter_base_set_end_stream(RpActiveStreamFilterBase* self,
                                                    bool end_stream);
void rp_active_stream_filter_base_set_processed_headers(RpActiveStreamFilterBase* self,
                                                        bool processed_headers);
bool rp_active_stream_filter_base_stopped_all(RpActiveStreamFilterBase* self);
RpIterationState_e rp_active_stream_filter_base_get_iteration_state(RpActiveStreamFilterBase* self);
void rp_active_stream_filter_base_set_iteration_state(RpActiveStreamFilterBase* self,
                                                        RpIterationState_e iteration_state);
void rp_active_stream_filter_base_send_local_reply(RpActiveStreamFilterBase* self,
                                                    evhtp_res code, evbuf_t* body,
                                                    modify_headers_cb modify_headers,
                                                    const char* details, void* arg);
static inline bool
rp_active_stream_filter_base_can_continue(RpActiveStreamFilterBase* self)
{
    return RP_IS_ACTIVE_STREAM_FILTER_BASE(self) ?
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->can_continue(self) : true;
}
static inline bool
rp_active_stream_filter_base_observed_end_stream(RpActiveStreamFilterBase* self)
{
    return RP_IS_ACTIVE_STREAM_FILTER_BASE(self) ?
        RP_ACTIVE_STREAM_FILTER_BASE_GET_CLASS(self)->observed_end_stream(self) : false;
}

G_END_DECLS
