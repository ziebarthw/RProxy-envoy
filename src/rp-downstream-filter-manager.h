/*
 * rp-downstream-filter-manager.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <glib-object.h>
#include "rp-dispatcher.h"
#include "rp-net-connection.h"
#include "rp-filter-manager.h"

G_BEGIN_DECLS

//source/common/local_reply/local_reply.h
#define RP_TYPE_LOCAL_REPLY rp_local_reply_get_type()
G_DECLARE_INTERFACE(RpLocalReply, rp_local_reply, RP, LOCAL_REPLY, GObject)

struct _RpLocalReplyInterface {
    GTypeInterface parent_iface;

    void (*rewrite)(RpLocalReply*,
                    const evhtp_headers_t*,
                    evhtp_headers_t*,
                    evhtp_res,
                    evbuf_t*,
                    const char*);
};

// The DownstreamFilterManager has explicit handling to send local replies.
// The UpstreamFilterManager will not, and will instead defer local reply
// management to the DownstreamFilterManager.
#define RP_TYPE_DOWNSTREAM_FILTER_MANAGER (rp_downstream_filter_manager_get_type())
G_DECLARE_FINAL_TYPE(RpDownstreamFilterManager, rp_downstream_filter_manager, RP, DOWNSTREAM_FILTER_MANAGER, RpFilterManager)

RpDownstreamFilterManager* rp_downstream_filter_manager_new(RpFilterManagerCallbacks* filter_manager_callbacks,
                                                            RpDispatcher* dispatcher,
                                                            RpNetworkConnection* connection,
                                                            guint64 stream_id,
                                                            bool proxy_100_continue,
                                                            guint32 buffer_limit,
                                                            RpFilterChainFactory* filter_chain_factory,
                                                            RpLocalReply* local_reply,
                                                            evhtp_proto protocol,
                                                            RpFilterState* parent_filter_state);
struct RpCreateChainResult rp_downstream_filter_manager_create_downstream_filter_chain(RpDownstreamFilterManager* self);
bool rp_downstream_filter_manager_has_last_downstream_byte_received(RpDownstreamFilterManager* self);

G_END_DECLS
