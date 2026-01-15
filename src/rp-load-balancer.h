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
    RpHostConstSharedPtr m_host;
    const char* m_details;
    RpAsyncHostSelectionHandle* m_cancelable;
};

static inline RpHostSelectionResponse
rp_host_selection_response_ctor(RpHostConstSharedPtr host, RpAsyncHostSelectionHandle* cancelable, const char* details)
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

RP_DEFINE_PAIR_STRICT(RpOverrideHost, const char*, bool);
RP_DECLARE_PAIR_CTOR(RpOverrideHost, const char*, bool);

struct _RpLoadBalancerContextInterface {
    GTypeInterface parent_iface;

    RpNetworkConnection* (*downstream_connection)(RpLoadBalancerContext*);
    RpStreamInfo* (*request_stream_info)(RpLoadBalancerContext*);
    evhtp_headers_t* (*downstream_headers)(RpLoadBalancerContext*);
    bool (*should_select_another_host)(RpLoadBalancerContext*, RpHost*);
    guint32 (*host_selection_retry_count)(RpLoadBalancerContext*);
    RpOverrideHost (*override_host_to_select)(RpLoadBalancerContext*);
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
static inline RpOverrideHost
rp_load_balancer_context_override_host_to_select(RpLoadBalancerContext* self)
{
    return RP_IS_LOAD_BALANCER_CONTEXT(self) ?
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->override_host_to_select(self) :
        RpOverrideHost_make(NULL, false);
}
static inline void
rp_load_balancer_context_on_async_host_selection(RpLoadBalancerContext* self, RpHost* host, const char* details)
{
    if (RP_IS_LOAD_BALANCER_CONTEXT(self)) \
        RP_LOAD_BALANCER_CONTEXT_GET_IFACE(self)->on_async_host_selection(self, host, details);
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

typedef UNIQUE_PTR(RpLoadBalancer) RpLoadBalancerPtr;

static inline RpHostSelectionResponse
rp_load_balancer_choose_host(RpLoadBalancer* self, RpLoadBalancerContext* context)
{
    return RP_IS_LOAD_BALANCER(self) ?
        RP_LOAD_BALANCER_GET_IFACE(self)->choose_host(self, context) :
        rp_host_selection_response_ctor(NULL, NULL, NULL);
}


typedef struct _RpLoadBalancerParams RpLoadBalancerParams;
struct _RpLoadBalancerParams {
    const RpPrioritySet* priority_set;
    const RpPrioritySet* local_priority_set;
};

/**
 * Factory for load balancers.
 */
#define RP_TYPE_LOAD_BALANCER_FACTORY rp_load_balancer_factory_get_type()
G_DECLARE_INTERFACE(RpLoadBalancerFactory, rp_load_balancer_factory, RP, LOAD_BALANCER_FACTORY, GObject)

struct _RpLoadBalancerFactoryInterface {
    GTypeInterface parent_iface;

    RpLoadBalancerPtr (*create)(RpLoadBalancerFactory*, RpLoadBalancerParams*);
    bool (*recreate_on_host_change)(RpLoadBalancerFactory*);
};

static inline RpLoadBalancerPtr
rp_load_balancer_factory_create(RpLoadBalancerFactory* self, RpLoadBalancerParams* params)
{
    return RP_IS_LOAD_BALANCER_FACTORY(self) ?
        RP_LOAD_BALANCER_FACTORY_GET_IFACE(self)->create(self, params) : NULL;
}
static inline bool
rp_load_balancer_factory_recreate_on_host_change(RpLoadBalancerFactory* self)
{
    return RP_IS_LOAD_BALANCER_FACTORY(self) ?
        RP_LOAD_BALANCER_FACTORY_GET_IFACE(self)->recreate_on_host_change(self) :
        true;
}


typedef SHARED_PTR(RpLoadBalancerFactory) RpLoadBalancerFactorySharedPtr;

/**
 * A thread aware load balancer is a load balancer that is global to all workers on behalf of a
 * cluster. These load balancers are harder to write so not every load balancer has to be one.
 * If a load balancer is a thread aware load balancer, the following semantics are used:
 * 1) A single instance is created on the main thread.
 * 2) The shared factory is passed to all workers.
 * 3) Every time there is a host set update on the main thread, all workers will create a new
 *    worker local load balancer via the factory.
 *
 * The above semantics mean that any global state in the factory must be protected by appropriate
 * locks. Additionally, the factory *must not* refer back to the owning thread aware load
 * balancer. If a cluster is removed via CDS, the thread aware load balancer can be destroyed
 * before cluster destruction reaches each worker. See the ring hash load balancer for one
 * example of how this pattern is used in practice. The common expected pattern is that the
 * factory will be consuming shared immutable state from the main thread
 *
 * TODO(mattklein123): The reason that locking is used in the above threading model vs. pure TLS
 * has to do with the lack of a TLS function that does the following:
 * 1) Create a per-worker data structure on the main thread. E.g., allocate 4 objects for 4
 *    workers.
 * 2) Then fan those objects out to each worker.
 * With the existence of a function like that, the callback locking from the worker to the main
 * thread could be removed. We can look at this in a follow up. The reality though is that the
 * locking is currently only used to protect some small bits of data on host set update and will
 * never be contended.
 */
#define RP_TYPE_THREAD_AWARE_LOAD_BALANCER rp_thread_aware_load_balancer_get_type()
G_DECLARE_INTERFACE(RpThreadAwareLoadBalancer, rp_thread_aware_load_balancer, RP, THREAD_AWARE_LOAD_BALANCER, GObject)

struct _RpThreadAwareLoadBalancerInterface {
    GTypeInterface parent_iface;

    RpLoadBalancerFactorySharedPtr (*factory)(RpThreadAwareLoadBalancer*);
    RpStatusCode_e (*initialize)(RpThreadAwareLoadBalancer*);
};

static inline RpLoadBalancerFactorySharedPtr
rp_thread_aware_load_balancer_factory(RpThreadAwareLoadBalancer* self)
{
    return RP_IS_THREAD_AWARE_LOAD_BALANCER(self) ?
        RP_THREAD_AWARE_LOAD_BALANCER_GET_IFACE(self)->factory(self) : NULL;
}
static inline RpStatusCode_e
rp_thread_aware_load_balancer_initialize(RpThreadAwareLoadBalancer* self)
{
    return RP_IS_THREAD_AWARE_LOAD_BALANCER(self) ?
        RP_THREAD_AWARE_LOAD_BALANCER_GET_IFACE(self)->initialize(self) :
        RpStatusCode_EnvoyOverloadError;
}

typedef UNIQUE_PTR(RpThreadAwareLoadBalancer) RpThreadAwareLoadBalancerPtr;


/*
 * Parsed load balancer configuration that will be used to create load balancer.
 */
#define RP_TYPE_LOAD_BALANCER_CONFIG rp_load_balancer_config_get_type()
G_DECLARE_INTERFACE(RpLoadBalancerConfig, rp_load_balancer_config, RP, LOAD_BALANCER_CONFIG, GObject)

struct _RpLoadBalancerConfigInterface {
    GTypeInterface parent_iface;

};

typedef UNIQUE_PTR(RpLoadBalancerConfig) RpLoadBalancerConfigPtr;

G_END_DECLS
