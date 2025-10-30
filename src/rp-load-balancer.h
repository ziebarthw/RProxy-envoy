/*
 * rp-load-balancer.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"
#include "rp-router.h"
#include "rp-stream-info.h"
#include "rp-upstream.h"

G_BEGIN_DECLS

/*
 * A handle to allow cancelation of asynchronous host selection.
 * If chooseHost returns a HostSelectionResponse with an AsyncHostSelectionHandle
 * handle, and the endpoint does not wish to receive onAsyncHostSelction call,
 * it must call cancel() on the provided handle.
 *
 * Please note that the AsyncHostSelectionHandle may be deleted after the
 * cancel() call. It is up to the implemention of the asynchronous load balancer
 * to ensure the cancelation state persists until the load balancer checks it.
 */
#define RP_TYPE_ASYNC_HOST_SELECTION_HANDLE rp_async_host_selection_handle_get_type()
G_DECLARE_INTERFACE(RpAsyncHostSelectionHandle, rp_async_host_selection_handle, RP, ASYNC_HOST_SELECTION_HANDLE, GObject)

struct _RpAsyncHostSelectionHandleInterface {
    GTypeInterface parent_iface;

    void (*cancel)(RpAsyncHostSelectionHandle*);
};

static inline void
rp_async_host_selection_handle_cancel(RpAsyncHostSelectionHandle* self)
{
    if (RP_IS_ASYNC_HOST_SELECTION_HANDLE(self))
    {
        RP_ASYNC_HOST_SELECTION_HANDLE_GET_IFACE(self)->cancel(self);
    }
}


/*
 * The response to a LoadBalancer::chooseHost call.
 *
 * chooseHost either returns a host directly or, in the case of asynchronous
 * load balancing, returns an AsyncHostSelectionHandle handle.
 *
 * If it returns a AsyncHostSelectionHandle handle, the load balancer guarantees an
 * eventual call to LoadBalancerContext::onAsyncHostSelction unless
 * AsyncHostSelectionHandle::cancel is called.
 */
typedef struct _RpHostSelectionResponse RpHostSelectionResponse;
struct _RpHostSelectionResponse {
    RpHost* m_host;
    const char* m_details;
    RpAsyncHostSelectionHandle* m_cancelable;
};

static inline RpHostSelectionResponse
rp_host_selection_response_ctor(RpHost* host, RpAsyncHostSelectionHandle* cancelable, const char* details)
{
    struct _RpHostSelectionResponse self = {
        .m_host = host,
        .m_details = details,
        .m_cancelable = cancelable
    };
    return self;
}


/**
 * Context information passed to a load balancer to use when choosing a host. Not all load
 * balancers make use of all context information.
 */
#define RP_TYPE_LOAD_BALANCER_CONTEXT rp_load_balancer_context_get_type()
G_DECLARE_INTERFACE(RpLoadBalancerContext, rp_load_balancer_context, RP, LOAD_BALANCER_CONTEXT, GObject)

struct _RpLoadBalancerContextInterface {
    GTypeInterface parent_iface;

    RpNetworkConnection* (*downstream_connection)(RpLoadBalancerContext*);
    RpStreamInfo* (*request_stream_info)(RpLoadBalancerContext*);
    evhtp_headers_t* (*downstream_headers)(RpLoadBalancerContext*);
    bool (*should_select_another_host)(RpLoadBalancerContext*, RpHost*);
    guint32 (*host_selection_retry_count)(RpLoadBalancerContext*);
    //TODO...absl::optional<OverrideHost> overrideHostToSelect() const PURE;
    void (*on_async_host_selection)(RpLoadBalancerContext*, RpHost*, const char*);
};

static inline RpNetworkConnection*
rp_load_balancer_context_downstream_connection(RpLoadBalancerContext* self)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->downstream_connection(self) :
        NULL;
}
static inline RpStreamInfo*
rp_load_balancer_context_request_stream_info(RpLoadBalancerContext* self)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->request_stream_info(self) :
        NULL;
}
static inline evhtp_headers_t*
rp_load_balancer_context_downstream_headers(RpLoadBalancerContext* self)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->downstream_headers(self) :
        NULL;
}
static inline bool
rp_load_balancer_context_should_select_another_host(RpLoadBalancerContext* self, RpHost* host)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->should_select_another_host(self, host) :
        false;
}
static inline guint32
rp_load_balancer_context_host_selection_retry_count(RpLoadBalancerContext* self)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->host_selection_retry_count(self) :
        0;
}
static inline void
rp_load_balancer_context_on_async_host_selection(RpLoadBalancerContext* self, RpHost* host, const char* details)
{
    if (RP_IS_LOAD_BALANCER_CONTEXT(self))
    {
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->on_async_host_selection(self, host, details);
    }
}


/**
 * Abstract load balancing interface.
 */
#define RP_TYPE_LOAD_BALANCER rp_load_balancer_get_type()
G_DECLARE_INTERFACE(RpLoadBalancer, rp_load_balancer, RP, LOAD_BALANCER, GObject)

struct _RpLoadBalancerInterface {
    GTypeInterface parent_iface;

    RpHostSelectionResponse (*choose_host)(RpLoadBalancer*, RpLoadBalancerContext*);
    //TODO...
    //TODO...selectExistingConnection(...);
};

static inline RpHostSelectionResponse
rp_load_balancer_choose_host(RpLoadBalancer* self, RpLoadBalancerContext* context)
{
    return RP_IS_LOAD_BALANCER(self) ?
        RP_LOAD_BALANCER_GET_IFACE(self)->choose_host(self, context) :
        rp_host_selection_response_ctor(NULL, NULL, NULL);
}

G_END_DECLS
