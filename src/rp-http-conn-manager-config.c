/*
 * rp-http-conn-manager-config.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_http_conn_manager_config_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_http_conn_manager_config_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "http1/rp-http1-server-connection-impl.h"
#include "router/rp-router-filter.h"
#include "router/rp-static-route-config-provider-impl.h"
#include "rp-conn-manager-config.h"
#include "rp-decompressor-filter.h"
#include "rp-rewrite-urls-filter.h"
#include "rp-state-filter.h"
#include "rp-http-conn-manager-config.h"

typedef enum {
    CodecType_HTTP1,
    CodecType_HTTP2,
    CodecType_HTTP3,
    CodecType_AUTO,
    CodecType_Unknown
} CodecType_e;

// using FilterConfigProvider = Envoy::Config::ExtensionConfigProvider<FactoryCb>;
// using FilterConfigProviderPtr = std::unique_ptr<FilterConfigProvider<FactoryCb>>;

// using Http::FilterFactoryCb = std::function<void(FilterChainFactoryCallbacks& callbacks)>;
// using HttpFilterFactoryCb = Http::FilterFactoryCb;

// class FilterChainUtility {
// public:
typedef struct _FilterFactoryProvider FilterFactoryProvider;
typedef struct _FilterFactoryProvider * FilterFactoryProviderPtr;
struct _FilterFactoryProvider {
    RpFilterFactoryCb* m_provider;
//    struct FilterConfigProvider/*<Filter::HttpFilterFactoryCb>*/ m_provider;
    // Filter::FilterConfigProviderPtr<Filter::HttpFilterFactoryCb> provider;
    bool m_disabled;
};
static inline FilterFactoryProvider
filter_factory_provider_ctor(RpFilterFactoryCb* provider, bool disabled)
{
    FilterFactoryProvider self = {
        .m_provider = provider,
        .m_disabled = disabled
    };
    return self;
}
static inline FilterFactoryProviderPtr
filter_factory_provider_new(RpFilterFactoryCb* provider, bool disabled)
{
    NOISY_MSG_("(%p(%p), %u)", provider, provider->m_free_func, disabled);
    FilterFactoryProviderPtr self = g_new0(FilterFactoryProvider, 1);
    *self = filter_factory_provider_ctor(provider, disabled);
    return self;
}
static void
filter_factory_provider_free(FilterFactoryProviderPtr self)
{
    NOISY_MSG_("(%p)", self);
    RpFilterFactoryCb* provider = self->m_provider;
    provider->m_free_func(provider);
    g_free(self);
}

struct _RpHttpConnectionManagerConfig {
    GObject parent_instance;

    RpHttpConnectionManagerCfg m_config;

    struct RpHttp1Settings_s m_http1_settings;

    RpLocalReply* m_local_reply;
    RpFactoryContext* m_context;

    RpRouteConfigProvider* m_route_config_provider;

    GSList/*<FilterFactoriesList>*/* m_filter_factories;
    // using FilterFactoriesList = std::vector<FilterFactoryProvider>;

    CodecType_e m_codec_type;
    guint32 m_max_request_headers_kb;
    guint32 m_max_request_headers_count;
    guint64 m_max_requests_per_connection;

    bool m_proxy_100_continue : 1;
};

static const guint64 stream_idle_timeout_ms = 5 * 60 * 1000;
static const guint64 request_timeout_ms = 0;
static const guint64 request_header_timeout_ms = 0;

static void filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface);
static void connection_manager_config_iface_init(RpConnectionManagerConfigInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpHttpConnectionManagerConfig, rp_http_connection_manager_config, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY, filter_chain_factory_iface_init)
    G_IMPLEMENT_INTERFACE(RP_TYPE_CONNECTION_MANAGER_CONFIG, connection_manager_config_iface_init)
)

static bool
create_filter_chain_i(RpFilterChainFactory* self, RpFilterChainManager* manager)
{
    NOISY_MSG_("(%p, %p)", self, manager);

    RpHttpConnectionManagerConfig* me = RP_HTTP_CONNECTION_MANAGER_CONFIG(self);
    for (GSList* itr = me->m_filter_factories; itr; itr = itr->next)
    {
        FilterFactoryProviderPtr factory = itr->data;
        if (factory->m_disabled)
        {
            NOISY_MSG_("disabled");
            continue;
        }

        rp_filter_chain_manager_apply_filter_factory_cb(manager, NULL, factory->m_provider);
    }

    return true;
}

static void
filter_chain_factory_iface_init(RpFilterChainFactoryInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_filter_chain = create_filter_chain_i;
}

static RpHttpServerConnection*
create_codec_i(RpConnectionManagerConfig* self, RpNetworkConnection* connection, evbuf_t* data G_GNUC_UNUSED, RpHttpServerConnectionCallbacks* callbacks)
{
    NOISY_MSG_("(%p, %p, %p(%zu), %p)", self, connection, data, evbuffer_get_length(data), callbacks);

    RpHttpConnectionManagerConfig* me = RP_HTTP_CONNECTION_MANAGER_CONFIG(self);
    switch (me->m_codec_type)
    {
        case CodecType_HTTP1:
            return RP_HTTP_SERVER_CONNECTION(
                rp_http1_server_connection_impl_new(connection,
                                                    callbacks,
                                                    &me->m_http1_settings,
                                                    rp_connection_manager_config_max_request_headers_kb(self),
                                                    rp_connection_manager_config_max_request_headers_count(self))
            );
        case CodecType_HTTP2:
        case CodecType_HTTP3:
        case CodecType_AUTO:
        case CodecType_Unknown:
            break;
    }
    LOGD("not implemented");
    return NULL;
}

static RpFilterChainFactory*
filter_factory_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_FILTER_CHAIN_FACTORY(self);
}

static guint32
max_request_headers_kb_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_max_request_headers_kb;
}

static guint32
max_request_headers_count_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_max_request_headers_count;
}

static guint64
max_requests_per_connection_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_max_requests_per_connection;
}

static bool
is_routable_i(RpConnectionManagerConfig* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
    return true;
}

static RpLocalReply*
local_reply_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_local_reply;
}

static bool
proxy_100_continue_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_proxy_100_continue;
}

static RpRouteConfigProvider*
route_config_provider_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_ROUTE_CONFIG_PROVIDER(RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_route_config_provider);
}

static lztq*
rules_i(RpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_HTTP_CONNECTION_MANAGER_CONFIG(self)->m_config.rules;
}

static void
connection_manager_config_iface_init(RpConnectionManagerConfigInterface* iface)
{
    LOGD("(%p)", iface);
    iface->create_codec = create_codec_i;
    iface->filter_factory = filter_factory_i;
    iface->max_request_headers_kb = max_request_headers_kb_i;
    iface->max_request_headers_count = max_request_headers_count_i;
    iface->max_requests_per_connection = max_requests_per_connection_i;
    iface->is_routable = is_routable_i;
    iface->local_reply = local_reply_i;
    iface->proxy_100_continue = proxy_100_continue_i;
    iface->route_config_provider = route_config_provider_i;
    iface->rules = rules_i;
}

static struct RpHttp1Settings_s
parse_http1_settings(void)
{
    LOGD("()");
    // Placeholder for more complete logic utilizing cfg stuff...
    return RpHttp1Settings;
}

static inline CodecType_e
get_codec_type(const char* codec_type)
{
    NOISY_MSG_("(%p(%s))", codec_type, codec_type);
    if (g_ascii_strcasecmp(codec_type, "AUTO") == 0)  {return CodecType_AUTO;}
    if (g_ascii_strcasecmp(codec_type, "HTTP1") == 0) {return CodecType_HTTP1;}
    if (g_ascii_strcasecmp(codec_type, "HTTP2") == 0) {return CodecType_HTTP2;}
    if (g_ascii_strcasecmp(codec_type, "HTTP3") == 0) {return CodecType_HTTP3;}
    return CodecType_Unknown;
}

static inline void
add_filter_factory(RpHttpConnectionManagerConfig* self, RpFilterFactoryCb* cb, bool disabled)
{
    NOISY_MSG_("(%p, %p(%p), %u)", self, cb, cb->m_free_func, disabled);
    self->m_filter_factories = g_slist_append(self->m_filter_factories, filter_factory_provider_new(cb, disabled));
}

#define GENERIC_FACTORY_CONTEXT(s) RP_GENERIC_FACTORY_CONTEXT((s)->m_context)
#define SERVER_FACTORY_CONTEXT(s) \
    rp_generic_factory_context_server_factory_context(GENERIC_FACTORY_CONTEXT(s))

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpHttpConnectionManagerConfig* self = RP_HTTP_CONNECTION_MANAGER_CONFIG(obj);
    g_slist_free_full(g_steal_pointer(&self->m_filter_factories), (GDestroyNotify)filter_factory_provider_free);

    G_OBJECT_CLASS(rp_http_connection_manager_config_parent_class)->dispose(obj);
}

static void
rp_http_connection_manager_config_class_init(RpHttpConnectionManagerConfigClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_http_connection_manager_config_init(RpHttpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);
    self->m_filter_factories = NULL;
}

static inline RpHttpConnectionManagerConfig*
constructed(RpHttpConnectionManagerConfig* self)
{
    NOISY_MSG_("(%p)", self);

    RpHttpConnectionManagerCfg* config = &self->m_config;

    self->m_http1_settings = parse_http1_settings();
    self->m_max_request_headers_count = config->http_protocol_options.max_headers_count;
    self->m_codec_type = get_codec_type(config->codec_type);

    self->m_route_config_provider = RP_ROUTE_CONFIG_PROVIDER(
        rp_static_route_config_provider_impl_new(&self->m_config.route_config,
                                                    SERVER_FACTORY_CONTEXT(self)));
    return self;
}

RpHttpConnectionManagerConfig*
rp_http_connection_manager_config_new(RpHttpConnectionManagerCfg* config, RpFactoryContext* context)
{
    LOGD("(%p, %p)", config, context);
    RpHttpConnectionManagerConfig* self = g_object_new(RP_TYPE_HTTP_CONNECTION_MANAGER_CONFIG, NULL);
    self->m_config = *config;
    self->m_context = context;
    return constructed(self);
}

void
rp_http_connection_manager_config_add_filter_factory(RpHttpConnectionManagerConfig* self, RpFilterFactoryCb* cb, bool disabled)
{
    LOGD("(%p, %p(%p), %u)", self, cb, cb->m_free_func, disabled);
    g_return_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_CONFIG(self));
    g_return_if_fail(cb != NULL);
    add_filter_factory(self, cb, disabled);
}

RpFactoryContext*
rp_http_connection_manager_config_factory_context(RpHttpConnectionManagerConfig* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_HTTP_CONNECTION_MANAGER_CONFIG(self), NULL);
    return self->m_context;
}
