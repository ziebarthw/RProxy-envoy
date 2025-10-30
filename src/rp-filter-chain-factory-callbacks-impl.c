/*
 * rp-filter-chain-factory-callbacks-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rp-active-stream-decoder-filter.h"
#include "rp-filter-chain-factory-callbacks-impl.h"

#if (defined(rp_filter_chain_factory_callbacks_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_filter_chain_factory_callbacks_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

struct _RpFilterChainFactoryCallbacksImpl {
    GObject parent_instance;

    RpFilterManager* m_manager;
    const struct RpFilterContext* m_context;
};

static void filter_chain_factory_callbacks_iface_init(RpFilterChainFactoryCallbacksInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpFilterChainFactoryCallbacksImpl, rp_filter_chain_factory_callbacks_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_CHAIN_FACTORY_CALLBACKS, filter_chain_factory_callbacks_iface_init)
)

static void
add_stream_filter(RpFilterChainFactoryCallbacks* self, RpStreamFilterBase* filter)
{
    NOISY_MSG_("(%p, %p)", self, filter);
    GList** filters = rp_filter_manager_get_filters(RP_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL(self)->m_manager);
    if (!g_list_find(*filters, filter))
    {
        *filters = g_list_append(*filters, filter);
    }
}

static void
add_stream_decoder_filter_i(RpFilterChainFactoryCallbacks* self, RpStreamDecoderFilter* filter)
{
    NOISY_MSG_("(%p, %p)", self, filter);
    add_stream_filter(self, RP_STREAM_FILTER_BASE(filter));
    RpFilterChainFactoryCallbacksImpl* me = RP_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL(self);
    RpFilterManager* manager = me->m_manager;
    const struct RpFilterContext* context = me->m_context;
    GList** decoder_filters = rp_filter_manager_get_decoder_filters(manager);
    RpActiveStreamDecoderFilter* decoder = rp_active_stream_decoder_filter_new(manager, filter, context);
    *decoder_filters = g_list_append(*decoder_filters, decoder);
}

static void
add_stream_encoder_filter_i(RpFilterChainFactoryCallbacks* self, RpStreamEncoderFilter* filter)
{
    NOISY_MSG_("(%p, %p)", self, filter);
    add_stream_filter(self, RP_STREAM_FILTER_BASE(filter));
    RpFilterChainFactoryCallbacksImpl* me = RP_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL(self);
    RpFilterManager* manager = me->m_manager;
    const struct RpFilterContext* context = me->m_context;
    GList** encoder_filters = rp_filter_manager_get_encoder_filters(manager);
    RpActiveStreamEncoderFilter* encoder = rp_active_stream_encoder_filter_new(manager, filter, context);
    *encoder_filters = g_list_append(*encoder_filters, encoder);
}

static void
add_stream_filter_i(RpFilterChainFactoryCallbacks* self, RpStreamFilter* filter)
{
    NOISY_MSG_("(%p, %p)", self, filter);
    add_stream_decoder_filter_i(self, RP_STREAM_DECODER_FILTER(filter));
    add_stream_encoder_filter_i(self, RP_STREAM_ENCODER_FILTER(filter));
}

static RpDispatcher*
dispatcher_i(RpFilterChainFactoryCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_filter_manager_dispatcher(RP_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL(self)->m_manager);
}

static RpFilterManager*
filter_manager_i(RpFilterChainFactoryCallbacks* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL(self)->m_manager;
}

static void
filter_chain_factory_callbacks_iface_init(RpFilterChainFactoryCallbacksInterface* iface)
{
    LOGD("(%p)", iface);
    iface->add_stream_decoder_filter = add_stream_decoder_filter_i;
    iface->add_stream_encoder_filter = add_stream_encoder_filter_i;
    iface->add_stream_filter = add_stream_filter_i;
    iface->dispatcher = dispatcher_i;
    iface->filter_manager = filter_manager_i;
}

OVERRIDE void
dispose(GObject* object)
{
    NOISY_MSG_("(%p)", object);
    G_OBJECT_CLASS(rp_filter_chain_factory_callbacks_impl_parent_class)->dispose(object);
}

static void
rp_filter_chain_factory_callbacks_impl_class_init(RpFilterChainFactoryCallbacksImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_filter_chain_factory_callbacks_impl_init(RpFilterChainFactoryCallbacksImpl* self)
{
    NOISY_MSG_("(%p)", self);
}

RpFilterChainFactoryCallbacksImpl*
rp_filter_chain_factory_callbacks_impl_new(RpFilterManager* manager, const struct RpFilterContext* context)
{
    LOGD("(%p, %p)", manager, context);
    g_return_val_if_fail(RP_IS_FILTER_MANAGER(manager), NULL);
    RpFilterChainFactoryCallbacksImpl* self = g_object_new(RP_TYPE_FILTER_CHAIN_FACTORY_CALLBACKS_IMPL, NULL);
    self->m_manager = manager;
    self->m_context = context;
    return self;
}
