/*
 * rp-state-filter.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_state_filter_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_state_filter_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-http-utility.h"
#include "rp-filter-chain-factory-callbacks-impl.h"
#include "rp-filter-manager.h"
#include "rp-state-filter.h"

#define DECODER_FILTER_CALLBACKS(s) \
    rp_pass_through_filter_decoder_callbacks_(RP_PASS_THROUGH_FILTER(s))
#define STREAM_FILTER_CALLBACKS(s) \
        RP_STREAM_FILTER_CALLBACKS(DECODER_FILTER_CALLBACKS(s))
#define STREAM_INFO(s) \
    rp_stream_filter_callbacks_stream_info(STREAM_FILTER_CALLBACKS(s))
#define FILTER_STATE(s) rp_stream_info_filter_state(STREAM_INFO(s))
#define RULES(s) \
    rp_filter_manager_callbacks_rules(\
        rp_filter_manager_filter_manager_callbacks(RP_STATE_FILTER(s)->m_filter_manager))
#define ROUTE(s) rp_stream_filter_callbacks_route(STREAM_FILTER_CALLBACKS(s))
#define ROUTE_ENTRY(s) rp_route_route_entry(ROUTE(s))
#define FILTER_MANAGER(c) \
    rp_filter_chain_factory_callbacks_filter_manager(c)

const char* rewrite_urls_key = "rp.state.filter.rewrite-urls";
const char* rule_key = "rp.state.filter.rule";
const char* original_uri_key = "rp.state.filter.original-uri";
const char* passthrough_key = "rp.state.filter.passthrough";
const char* dynamic_host_key = "rp.state.filter.dynamic-host";
const char* dynamic_port_key = "rp.state.filter.dynamic-port";

struct _RpStateFilter {
    RpPassThroughFilter parent_instance;

    SHARED_PTR(RpFilterManager) m_filter_manager;
    UNIQUE_PTR(char) m_original_uri;
};

static inline void stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpStateFilter, rp_state_filter, RP_TYPE_PASS_THROUGH_FILTER,
    G_IMPLEMENT_INTERFACE(RP_TYPE_STREAM_DECODER_FILTER, stream_decoder_filter_iface_init)
)

#define PARENT_STREAM_DECODER_FILTER_IFACE(s) \
    ((RpStreamDecoderFilterInterface*)g_type_interface_peek_parent(RP_STREAM_DECODER_FILTER_GET_IFACE(s)))

static inline rule_cfg_t*
get_rule_cfg(RpStateFilter* self)
{
    NOISY_MSG_("(%p)", self);
    rule_cfg_t* rule_cfg;
    g_object_get(G_OBJECT(ROUTE(self)), "rule-cfg", &rule_cfg, NULL);
    return rule_cfg;
}

static inline rule_t*
find_rule_from_cfg(rule_cfg_t* rule_cfg, lztq* rules)
{
    NOISY_MSG_("(%p, %p)", rule_cfg, rules);

    for (lztq_elem* elem = lztq_first(rules); elem; elem = lztq_next(elem))
    {
        rule_t* rule = lztq_elem_data(elem);

        if (rule->config == rule_cfg)
        {
            NOISY_MSG_("found rule %p", rule);
            return rule;
        }
    }

    LOGI("not found");
    return NULL;
}

static inline rule_t*
get_rule(RpStateFilter* self)
{
    NOISY_MSG_("(%p)", self);
    return find_rule_from_cfg(get_rule_cfg(self), RULES(self));
}

static inline lztq*
get_rewrite_urls(rule_t* rule)
{
    NOISY_MSG_("(%p)", rule);
    return rule->parent_vhost ? rule->parent_vhost->config->rewrite_urls : NULL;
}

static RpFilterHeadersStatus_e
decode_headers_i(RpStreamDecoderFilter* self, evhtp_headers_t* request_headers, bool end_stream)
{
    NOISY_MSG_("(%p(%s), %p, %u)", self, G_OBJECT_TYPE_NAME(self), request_headers, end_stream);

    RpStateFilter* me = RP_STATE_FILTER(self);
    RpFilterState* filter_state = FILTER_STATE(me);
    rule_t* rule = get_rule(me);

    if (rule->config->passthrough)
    {
        NOISY_MSG_("passthrough");
        rp_filter_state_set_data(filter_state,
                                    passthrough_key,
                                    (gpointer)passthrough_key,
                                    RpFilterStateStateType_ReadOnly,
                                    RpFilterStateLifeSpan_Request);
        return RpFilterHeadersStatus_Continue;
    }

    rp_filter_state_set_data(filter_state,
                                rule_key,
                                rule,
                                RpFilterStateStateType_ReadOnly,
                                RpFilterStateLifeSpan_Request);

    lztq* rewrite_urls = get_rewrite_urls(rule);
    rp_filter_state_set_data(filter_state,
                                rewrite_urls_key,
                                rewrite_urls,
                                RpFilterStateStateType_ReadOnly,
                                RpFilterStateLifeSpan_Request);

    char* original_uri = me->m_original_uri = http_utility_build_original_uri(request_headers);
    rp_filter_state_set_data(filter_state,
                                original_uri_key,
                                original_uri,
                                RpFilterStateStateType_ReadOnly,
                                RpFilterStateLifeSpan_Request);

    return RpFilterHeadersStatus_Continue;
}

static void
decode_complete_i(RpStreamDecoderFilter* self)
{
    NOISY_MSG_("(%p)", self);
    PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_complete(self);
}

static RpFilterDataStatus_e
decode_data_i(RpStreamDecoderFilter* self, evbuf_t* data, bool end_stream)
{
    NOISY_MSG_("(%p, %p(%zu), %u)", self, data, data ? evbuffer_get_length(data) : 0, end_stream);
    return PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_data(self, data, end_stream);
}

static RpFilterTrailerStatus_e
decode_trailers_i(RpStreamDecoderFilter* self, evhtp_headers_t* trailers)
{
    NOISY_MSG_("(%p, %p)", self, trailers);
    return PARENT_STREAM_DECODER_FILTER_IFACE(self)->decode_trailers(self, trailers);
}

static void
set_decoder_filter_callbacks_i(RpStreamDecoderFilter* self, RpStreamDecoderFilterCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);
    PARENT_STREAM_DECODER_FILTER_IFACE(self)->set_decoder_filter_callbacks(self, callbacks);
}

static inline void
stream_decoder_filter_iface_init(RpStreamDecoderFilterInterface* iface)
{
    LOGD("(%p)", iface);
    iface->decode_headers = decode_headers_i;
    iface->decode_complete = decode_complete_i;
    iface->decode_data = decode_data_i;
    iface->decode_trailers = decode_trailers_i;
    iface->set_decoder_filter_callbacks = set_decoder_filter_callbacks_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpStateFilter* self = RP_STATE_FILTER(obj);
    g_clear_pointer(&self->m_original_uri, g_free);

    G_OBJECT_CLASS(rp_state_filter_parent_class)->dispose(obj);
}

static void
rp_state_filter_class_init(RpStateFilterClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_state_filter_init(RpStateFilter* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpStateFilter*
state_filter_new(RpFilterManager* filter_manager)
{
    NOISY_MSG_("(%p)", filter_manager);
    RpStateFilter* self = g_object_new(RP_TYPE_STATE_FILTER, NULL);
    self->m_filter_manager = filter_manager;
    return self;
}

static void
filter_factory_cb(RpFilterFactoryCb* self, RpFilterChainFactoryCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p)", self, callbacks);

    RpStateFilter* filter = state_filter_new(FILTER_MANAGER(callbacks));
    rp_filter_chain_factory_callbacks_add_stream_decoder_filter(callbacks, RP_STREAM_DECODER_FILTER(filter));
}

RpFilterFactoryCb*
rp_state_filter_create_filter_factory(RpFactoryContext* context)
{
    LOGD("(%p)", context);
    g_return_val_if_fail(RP_IS_FACTORY_CONTEXT(context), NULL);
    return rp_filter_factory_cb_new(filter_factory_cb, g_free);
}
