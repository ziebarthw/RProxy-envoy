/*
 * rp-conn-pool.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rp-host-description.h"

G_BEGIN_DECLS

typedef struct _RpConnectionPoolInstance RpConnectionPoolInstance;
typedef void (*RpIdleCb)(RpConnectionPoolInstance*);

/**
 * Controls the behavior of a canceled stream.
 */
typedef enum {
    RpCancelPolicy_Default,
    RpCancelPolicy_CloseExcess,
} RpCancelPolicy_e;

/**
 * Controls the behavior when draining a connection pool.
 */
typedef enum {
    RpDrainBehavior_Undefined,
    RpDrainBehavior_DrainAndDelete,
    RpDrainBehavior_DrainExistingConnections,
} RpDrainBehavior_e;

typedef enum {
    RpPoolFailureReason_Overflow,
    RpPoolFailureReason_LocalConnectionFailure,
    RpPoolFailureReason_RemoteConnectionFailure,
    RpPoolFailureReason_Timeout,
} RpPoolFailureReason_e;


/**
 * Handle that allows a pending connection or stream to be canceled before it is completed.
 */
#define RP_TYPE_CANCELLABLE rp_cancellable_get_type()
G_DECLARE_INTERFACE(RpCancellable, rp_cancellable, RP, CANCELLABLE, GObject)

struct _RpCancellableInterface {
    GTypeInterface parent_iface;

    void (*cancel)(RpCancellable*, RpCancelPolicy_e);
};

static inline void
rp_cancellable_cancel(RpCancellable* self, RpCancelPolicy_e cancel_policy)
{
    if (RP_IS_CANCELLABLE(self))
    {
        RP_CANCELLABLE_GET_IFACE(self)->cancel(self, cancel_policy);
    }
}


/**
 * An instance of a generic connection pool.
 */
#define RP_TYPE_CONNECTION_POOL_INSTANCE rp_connection_pool_instance_get_type()
G_DECLARE_INTERFACE(RpConnectionPoolInstance, rp_connection_pool_instance, RP, CONNECTION_POOL_INSTANCE, GObject)

struct _RpConnectionPoolInstanceInterface {
    GTypeInterface parent_iface;

    void (*add_idle_callback)(RpConnectionPoolInstance*, RpIdleCb);
    bool (*is_idle)(RpConnectionPoolInstance*);
    void (*drain_connections)(RpConnectionPoolInstance*, RpDrainBehavior_e);
    RpHostDescriptionConstSharedPtr (*host)(RpConnectionPoolInstance*);
    bool (*maybe_preconnect)(RpConnectionPoolInstance*, float);
};

static inline void
rp_connection_pool_instance_add_idle_callback(RpConnectionPoolInstance* self, RpIdleCb cb)
{
    if (RP_IS_CONNECTION_POOL_INSTANCE(self))
    {
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->add_idle_callback(self, cb);
    }
}
static inline bool
rp_connection_pool_instance_is_idle(RpConnectionPoolInstance* self)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->is_idle(self) : false;
}
static inline void
rp_connection_pool_instance_drain_connections(RpConnectionPoolInstance* self, RpDrainBehavior_e drain_behavior)
{
    if (RP_IS_CONNECTION_POOL_INSTANCE(self))
    {
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->drain_connections(self, drain_behavior);
    }
}
static inline RpHostDescriptionConstSharedPtr
rp_connection_pool_instance_host(RpConnectionPoolInstance* self)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->host(self) : NULL;
}
static inline bool
rp_connection_pool_instance_maybe_preconnect(RpConnectionPoolInstance* self, float preconnect_ratio)
{
    return RP_IS_CONNECTION_POOL_INSTANCE(self) ?
        RP_CONNECTION_POOL_INSTANCE_GET_IFACE(self)->maybe_preconnect(self, preconnect_ratio) :
        false;
}

static inline void
append_u8(GByteArray* b, guint8 v)
{
    g_byte_array_append(b, &v, 1);
}
static inline void
append_u16_be(GByteArray* b, guint16 v)
{
    guint8 tmp[2] = { (guint8)(v >> 8), (guint8)(v & 0xff) };
    g_byte_array_append(b, tmp, 2);
}
static inline void
append_u32_be(GByteArray* b, guint32 v)
{
    guint8 tmp[4] = {
        (guint8)(v >> 24), (guint8)(v >> 16), (guint8)(v >> 8), (guint8)(v)
    };
    g_byte_array_append(b, tmp, 4);
}
static inline void
append_bytes_len(GByteArray* b, const guint8* p, gsize n)
{
    /* length-prefix so boundaries are unambiguous */
    g_return_if_fail(n <= G_MAXUINT32);
    append_u32_be(b, (guint32)n);
    if (n) g_byte_array_append(b, p, (guint)n);
}
static inline void
append_str_len(GByteArray* b, const char* s)
{
    if (!s) {
        append_u32_be(b, 0);
        return;
    }
    append_bytes_len(b, (const guint8*)s, strlen(s));
}

/* Sample of a stable, deterministic digest of options */
typedef struct _RpOptionsDigest RpOptionsDigest;
struct _RpOptionsDigest {
    guint8  digest[32]; /* e.g., SHA-256 */
    gsize   len;        /* 32 */
};

static inline GBytes*
rp_conn_pool_key_new(int seed, // evhtp_proto/priority
                     const guint8* alpn_bytes, gsize alpn_len,
                     const char* sni,
                     const RpOptionsDigest* opt_digest)
{
    GByteArray* b = g_byte_array_new();

    /* Version tag: lets you evolve format safely */
    append_u8(b, 1);

    /* Fixed-order fields */
    append_u8(b, (guint8)seed);
    append_bytes_len(b, alpn_bytes, alpn_len);
    append_str_len(b, sni);

    if (opt_digest)
    {
        append_u8(b, 1);
        append_bytes_len(b, opt_digest->digest, opt_digest->len);
    }
    else
    {
        append_u8(b, 0);
    }

    /* Freeze: GBytes owns the data now */
    return g_byte_array_free_to_bytes(b);
}

G_END_DECLS
