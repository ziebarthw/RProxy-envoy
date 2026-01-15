/*
 * rp-server-instance-base.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_server_instance_base_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_server_instance_base_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "event/rp-dispatcher-impl.h"
#include "event/rp-real-time-system.h"
#include "local_info/rp-local-info-impl.h"
#include "upstream/rp-cluster-manager-impl.h"
#include "server/rp-run-helper.h"
#include "server/rp-server-impl.h"
#include "server/rp-server-configuration-impl.h"
#include "singleton/rp-singleton-manager-impl.h"
#include "rp-thread-local.h"
#include "server/rp-server-instance-base.h"

typedef struct _RpServerInstanceBasePrivate RpServerInstanceBasePrivate;
struct _RpServerInstanceBasePrivate {

    RpLocalInfo* m_local_info;
    RpClusterManagerFactory* m_cluster_manager_factory;
    UNIQUE_PTR(RpServerFactoryContextImpl) m_server_contexts;
    UNIQUE_PTR(RpSingletonManagerImpl) m_singleton_manager;
    RpDispatcherPtr m_dispatcher;
    RpServerConfigurationMainImpl* m_config;

    RpThreadLocalInstance* m_thread_local;

    rproxy_t* m_bootstrap;
    GMutex m_lock;

    evthr_t* m_main_thread;

    bool m_terminated : 1;
    bool m_enable_reuse_port_default : 1;
    bool m_shutdown : 1;
};

enum
{
    PROP_0, // Reserved.
    PROP_BOOTSTRAP,
    PROP_TLS,
    PROP_THR,
    N_PROPERTIES
};

static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };

static void server_instance_iface_init(RpServerInstanceInterface* iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(RpServerInstanceBase, rp_server_instance_base, G_TYPE_OBJECT,
    G_ADD_PRIVATE(RpServerInstanceBase)
    G_IMPLEMENT_INTERFACE(RP_TYPE_SERVER_INSTANCE, server_instance_iface_init)
)

#define PRIV(obj) \
    ((RpServerInstanceBasePrivate*) rp_server_instance_base_get_instance_private(RP_SERVER_INSTANCE_BASE(obj)))

static inline void
terminate(RpServerInstanceBasePrivate* me)
{
    NOISY_MSG_("(%p)", me);

    if (me->m_terminated)
    {
        LOGD("already terminated!");
        return;
    }

    me->m_terminated = true;

    rp_thread_local_instance_shutdown_global_threading(me->m_thread_local);

    //TODO...

    rp_thread_local_instance_shutdown_thread(me->m_thread_local);
}

static void
run_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);

    RpServerInstanceBasePrivate* me = PRIV(self);
    g_autoptr(RpRunHelper) run_helper = rp_run_helper_new(self, me->m_dispatcher, rp_server_instance_cluster_manager(self));

    NOISY_MSG_("starting main dispatch loop");
    //TODO...
    rp_dispatcher_run(me->m_dispatcher, RpRunType_BLOCK);
    NOISY_MSG_("main dispatch loop exited");

    terminate(me);
}

static void
shutdown_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    RpServerInstanceBasePrivate* me = PRIV(self);
    me->m_shutdown = true;
    //TODO...
    rp_dispatcher_exit(me->m_dispatcher);
}

static bool
is_shutdown_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_shutdown;
}

static bool
enable_reuse_port_default_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_enable_reuse_port_default;
}

static RpServerFactoryContext*
server_factory_context_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SERVER_FACTORY_CONTEXT(PRIV(self)->m_server_contexts);
}

static RpTransportSocketFactoryContext*
transport_socket_factory_context_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_TRANSPORT_SOCKET_FACTORY_CONTEXT(PRIV(self)->m_server_contexts);
}

static RpClusterManager*
cluster_manager_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return rp_server_configuration_cluster_manager(RP_SERVER_CONFIGURATION_MAIN(PRIV(self)->m_config));
}

static RpLocalInfo*
local_info_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_local_info;
}

static RpDispatcher*
dispatcher_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_dispatcher;
}

static rproxy_t*
bootstrap_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_bootstrap;
}

static GMutex*
lock_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return &PRIV(self)->m_lock;
}

static RpSingletonManager*
singleton_manager_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_SINGLETON_MANAGER(PRIV(self)->m_singleton_manager);
}

static RpThreadLocalInstance*
thread_local_i(RpServerInstance* self)
{
    NOISY_MSG_("(%p)", self);
    return PRIV(self)->m_thread_local;
}

static void
server_instance_iface_init(RpServerInstanceInterface* iface)
{
    LOGD("(%p)", iface);
    iface->run = run_i;
    iface->shutdown = shutdown_i;
    iface->is_shutdown = is_shutdown_i;
    iface->enable_reuse_port_default = enable_reuse_port_default_i;
    iface->server_factory_context = server_factory_context_i;
    iface->transport_socket_factory_context = transport_socket_factory_context_i;
    iface->cluster_manager = cluster_manager_i;
    iface->local_info = local_info_i;
    iface->dispatcher = dispatcher_i;
    iface->bootstrap = bootstrap_i;
    iface->lock = lock_i;
    iface->singleton_manager = singleton_manager_i;
    iface->thread_local = thread_local_i;
}

OVERRIDE void
set_property(GObject* obj, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    NOISY_MSG_("(%p, %u, %p, %p(%s))", obj, prop_id, value, pspec, pspec->name);
    switch (prop_id)
    {
        case PROP_BOOTSTRAP:
            PRIV(obj)->m_bootstrap = g_value_get_pointer(value);
            break;
        case PROP_TLS:
            PRIV(obj)->m_thread_local = g_value_get_object(value);
            break;
        case PROP_THR:
            PRIV(obj)->m_main_thread = g_value_get_pointer(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, prop_id, pspec);
            break;
    }
}

static inline RpDispatcherPtr
allocate_dispatcher(evthr_t* main_thread)
{
    NOISY_MSG_("(%p)", main_thread);
    RpTimeSystem* time_system = RP_TIME_SYSTEM(rp_real_time_system_new());
    return rp_dispatcher_impl_new("main_thread", g_steal_pointer(&time_system), main_thread);
}

OVERRIDE void
constructed(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    G_OBJECT_CLASS(rp_server_instance_base_parent_class)->constructed(obj);

    RpServerInstanceBase* self = RP_SERVER_INSTANCE_BASE(obj);
    RpServerInstanceBasePrivate* me = PRIV(self);
    me->m_singleton_manager = rp_singleton_manager_impl_new();
    me->m_dispatcher = allocate_dispatcher(me->m_main_thread);
    // Implements both ServerFactoryContext and TransportSocketFactoryContext
    // interfaces.
    me->m_server_contexts = rp_server_factory_context_impl_new(RP_SERVER_INSTANCE(obj));
    me->m_config = rp_server_configuration_main_impl_new();
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpServerInstanceBasePrivate* me = PRIV(obj);

    terminate(me);

    g_clear_object(&me->m_server_contexts);
    g_clear_object(&me->m_config);
    g_clear_object(&me->m_dispatcher);
    g_clear_object(&me->m_singleton_manager);

    G_OBJECT_CLASS(rp_server_instance_base_parent_class)->dispose(obj);
}

static void
rp_server_instance_base_class_init(RpServerInstanceBaseClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->set_property = set_property;
    object_class->constructed = constructed;
    object_class->dispose = dispose;

    obj_properties[PROP_BOOTSTRAP] = g_param_spec_pointer("bootstrap",
                                                    "Bootstrap",
                                                    "rproxy_t instance",
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_TLS] = g_param_spec_object("tls",
                                                    "Thread local storage",
                                                    "ThreadLocalInstance instance",
                                                    RP_TYPE_THREAD_LOCAL_INSTANCE,
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_THR] = g_param_spec_pointer("thr",
                                                    "Thr",
                                                    "evthr_t instance",
                                                    G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties(object_class, N_PROPERTIES, obj_properties);
}

static void
rp_server_instance_base_init(RpServerInstanceBase* self)
{
    NOISY_MSG_("(%p)", self);

    RpServerInstanceBasePrivate* me = PRIV(self);
    g_mutex_init(&me->m_lock);
    me->m_enable_reuse_port_default = true;
    me->m_shutdown = false;
}

void
rp_server_instance_base_initialize(RpServerInstanceBase* self)
{
    LOGD("(%p)", self);

    RpServerInstanceBasePrivate* me = PRIV(self);

    rp_thread_local_instance_register_thread(me->m_thread_local, me->m_dispatcher, true);

    me->m_local_info = RP_LOCAL_INFO(rp_local_info_impl_new());
    me->m_cluster_manager_factory = RP_CLUSTER_MANAGER_FACTORY(
        rp_prod_cluster_manager_factory_new(
            rp_server_instance_server_factory_context(RP_SERVER_INSTANCE(self)), me->m_thread_local, RP_SERVER_INSTANCE(self)));

    g_autoptr(GError) err = NULL;
    if (!rp_server_configuration_main_impl_initialize(me->m_config, me->m_bootstrap, RP_SERVER_INSTANCE(self), me->m_cluster_manager_factory, &err))
    {
        LOGE("error %d(%s)", err->code, err->message);
        return;
    }
}

RpServerConfigurationMainImpl*
rp_server_instance_base_config(RpServerInstanceBase* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_SERVER_INSTANCE_BASE(self), NULL);
    return PRIV(self)->m_config;
}
