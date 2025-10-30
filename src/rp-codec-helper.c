/*
 * rp-codec-helper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_codec_helper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_codec_helper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-helper.h"

typedef struct _RpStreamCallbackHelperPrivate RpStreamCallbackHelperPrivate;
struct _RpStreamCallbackHelperPrivate {
    GArray* m_callbacks;
    guint32 m_high_watermark_callbacks;
    bool m_reset_callbacks_started : 1;
    bool m_local_end_stream : 1;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(RpStreamCallbackHelper, rp_stream_callback_helper, G_TYPE_OBJECT)

#define PRIV(obj) \
    ((RpStreamCallbackHelperPrivate*)rp_stream_callback_helper_get_instance_private(RP_STREAM_CALLBACK_HELPER(obj)))

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_stream_callback_helper_parent_class)->constructed(obj);

    RpStreamCallbackHelperPrivate* me = PRIV(obj);
    me->m_callbacks = g_array_sized_new(false, true, sizeof(RpStreamCallbacks*), 8);
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpStreamCallbackHelperPrivate* me = PRIV(obj);
    if (me->m_callbacks)
    {
        g_array_free(me->m_callbacks, true);
        me->m_callbacks = NULL;
    }

    G_OBJECT_CLASS(rp_stream_callback_helper_parent_class)->dispose(obj);
}

static void
rp_stream_callback_helper_class_init(RpStreamCallbackHelperClass* klass)
{
    NOISY_MSG_("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->constructed = constructed;
    object_class->dispose = dispose;
}

static void
rp_stream_callback_helper_init(RpStreamCallbackHelper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

void
rp_stream_callback_helper_add_callbacks_helper(RpStreamCallbackHelper* self, RpStreamCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_if_fail(RP_IS_STREAM_CALLBACK_HELPER(self));
    g_return_if_fail(RP_IS_STREAM_CALLBACKS(callbacks));
    RpStreamCallbackHelperPrivate* me = PRIV(self);
    me->m_callbacks = g_array_append_val(me->m_callbacks, callbacks);
    for (guint32 i=0; i < me->m_high_watermark_callbacks; ++i)
    {
        rp_stream_callbacks_on_above_write_buffer_high_watermark(callbacks);
    }
}

void
rp_stream_callback_helper_remove_callbacks_helper(RpStreamCallbackHelper* self, RpStreamCallbacks* callbacks)
{
    LOGD("(%p, %p)", self, callbacks);
    g_return_if_fail(RP_IS_STREAM_CALLBACK_HELPER(self));
    g_return_if_fail(RP_IS_STREAM_CALLBACKS(callbacks));
    RpStreamCallbackHelperPrivate* me = PRIV(self);
    for (guint i=0; i < me->m_callbacks->len; ++i)
    {
        RpStreamCallbacks* callback = g_array_index(me->m_callbacks, RpStreamCallbacks*, i);
        if (callback == callbacks)
        {
            g_array_index(me->m_callbacks, RpStreamCallbacks*, i) = NULL;
            return;
        }
    }
}

void
rp_stream_callback_helper_run_low_watermark_callbacks(RpStreamCallbackHelper* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_STREAM_CALLBACK_HELPER(self));
    RpStreamCallbackHelperPrivate* me = PRIV(self);
    if (me->m_reset_callbacks_started || me->m_local_end_stream)
    {
        return;
    }

    --me->m_high_watermark_callbacks;
    for (gint i=0; i < me->m_callbacks->len; ++i)
    {
        RpStreamCallbacks* callbacks = g_array_index(me->m_callbacks, RpStreamCallbacks*, i);
        if (callbacks)
        {
            rp_stream_callbacks_on_below_write_buffer_low_watermark(callbacks);
        }
    }
}

void
rp_stream_callback_helper_run_high_watermark_callbacks(RpStreamCallbackHelper* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(RP_IS_STREAM_CALLBACK_HELPER(self));
    RpStreamCallbackHelperPrivate* me = PRIV(self);
    if (me->m_reset_callbacks_started || me->m_local_end_stream)
    {
        return;
    }

    ++me->m_high_watermark_callbacks;
    for (gint i=0; i < me->m_callbacks->len; ++i)
    {
        RpStreamCallbacks* callbacks = g_array_index(me->m_callbacks, RpStreamCallbacks*, i);
        if (callbacks)
        {
            rp_stream_callbacks_on_above_write_buffer_high_watermark(callbacks);
        }
    }
}

void
rp_stream_callback_helper_run_reset_callbacks(RpStreamCallbackHelper* self, RpStreamResetReason_e reason, const char* details)
{
    LOGD("(%p, %d, %p(%s))", self, reason, details, details);
    g_return_if_fail(RP_IS_STREAM_CALLBACK_HELPER(self));
    RpStreamCallbackHelperPrivate* me = PRIV(self);
    if (me->m_reset_callbacks_started)
    {
        return;
    }

    me->m_reset_callbacks_started = true;
    for (gint i=0; i < me->m_callbacks->len; ++i)
    {
        RpStreamCallbacks* callbacks = g_array_index(me->m_callbacks, RpStreamCallbacks*, i);
        rp_stream_callbacks_on_reset_stream(callbacks, reason, details);
    }
}
