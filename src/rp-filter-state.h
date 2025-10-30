/*
 * rp-filter-state.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
    RpFilterStateStateType_ReadOnly,
    RpFilterStateStateType_Mutable
} RpFilterStateStateType_e;

typedef enum
{
    RpFilterStateLifeSpan_FilterChain,
    RpFilterStateLifeSpan_Request,
    RpFilterStateLifeSpan_Connection,
    RpFilterStateLifeSpan_TopSpan = RpFilterStateLifeSpan_Connection
} RpFilterStateLifeSpan_e;

/**
 * FilterState represents dynamically generated information regarding a stream (TCP or HTTP level)
 * or a connection by various filters in Envoy. FilterState can be write-once or write-many.
 */
#define RP_TYPE_FILTER_STATE rp_filter_state_get_type()
G_DECLARE_INTERFACE(RpFilterState, rp_filter_state, RP, FILTER_STATE, GObject)

struct _RpFilterStateInterface {
    GTypeInterface parent_iface;

    void (*set_data)(RpFilterState*,
                        const char*,
                        gpointer,
                        RpFilterStateStateType_e,
                        RpFilterStateLifeSpan_e);
    gpointer (*get_data)(RpFilterState*, const char*);
    bool (*has_data_with_name)(RpFilterState*, const char*);
    bool (*has_data_at_or_above_life_span)(RpFilterState*,
                                            RpFilterStateLifeSpan_e);
    RpFilterStateLifeSpan_e (*life_span)(RpFilterState*);
    RpFilterState* (*parent)(RpFilterState*);
};

static inline void
rp_filter_state_set_data(RpFilterState* self, const char* data_name, gpointer data, RpFilterStateStateType_e state_type, RpFilterStateLifeSpan_e life_span)
{
    if (RP_IS_FILTER_STATE(self)) \
        RP_FILTER_STATE_GET_IFACE(self)->set_data(self, data_name, data, state_type, life_span);
}
static inline gpointer
rp_filter_state_get_data(RpFilterState* self, const char* data_name)
{
    return RP_IS_FILTER_STATE(self) ?
        RP_FILTER_STATE_GET_IFACE(self)->get_data(self, data_name) : NULL;
}
static inline bool
rp_filter_state_has_data_with_name(RpFilterState* self, const char* data_name)
{
    return RP_IS_FILTER_STATE(self) ?
        RP_FILTER_STATE_GET_IFACE(self)->has_data_with_name(self, data_name) :
        false;
}
static inline bool
rp_filter_state_has_data_at_or_above_life_span(RpFilterState* self, RpFilterStateLifeSpan_e life_span)
{
    return RP_IS_FILTER_STATE(self) ?
        RP_FILTER_STATE_GET_IFACE(self)->has_data_at_or_above_life_span(self, life_span) :
        false;
}
static inline RpFilterStateLifeSpan_e
rp_filter_state_life_span(RpFilterState* self)
{
    return RP_IS_FILTER_STATE(self) ?
        RP_FILTER_STATE_GET_IFACE(self)->life_span(self) :
        RpFilterStateLifeSpan_FilterChain;
}
static inline RpFilterState*
rp_filter_state_parent(RpFilterState* self)
{
    return RP_IS_FILTER_STATE(self) ?
        RP_FILTER_STATE_GET_IFACE(self)->parent(self) : NULL;
}

G_END_DECLS
