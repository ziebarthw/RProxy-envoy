/*
 * rp-socket-interface-impl-factory.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_network_address_socket_interface_impl_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_network_address_socket_interface_impl_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "network/rp-socket-interface-impl.h"

SINGLETON_MANAGER_REGISTRATION(socket_interface);

struct _RpNetworkAddressSocketInterfaceImplFactory {
    GObject parent_instance;

    RpSingletonManager* m_singleton_manager;
};

G_DEFINE_TYPE(RpNetworkAddressSocketInterfaceImplFactory, rp_network_address_socket_interface_impl_factory, G_TYPE_OBJECT)

static RpSingletonInstanceSharedPtr
singleton_factory_cb(void)
{
    NOISY_MSG_("()");
    return RP_SINGLETON_INSTANCE(rp_network_address_socket_interface_impl_new());
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);
    G_OBJECT_CLASS(rp_network_address_socket_interface_impl_factory_parent_class)->dispose(obj);
}

static void
rp_network_address_socket_interface_impl_factory_class_init(RpNetworkAddressSocketInterfaceImplFactoryClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_network_address_socket_interface_impl_factory_init(RpNetworkAddressSocketInterfaceImplFactory* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpNetworkAddressSocketInterfaceImplFactory*
rp_network_address_socket_interface_impl_factory_new(RpSingletonManager* singleton_manager)
{
    LOGD("(%p)", singleton_manager);
    g_return_val_if_fail(RP_IS_SINGLETON_MANAGER(singleton_manager), NULL);
    RpNetworkAddressSocketInterfaceImplFactory* self = g_object_new(RP_TYPE_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL_FACTORY, NULL);
    self->m_singleton_manager = singleton_manager;
    return self;
}

RpNetworkAddressSocketInterface*
rp_network_address_socket_interface_impl_factory_get(RpNetworkAddressSocketInterfaceImplFactory* self)
{
    LOGD("(%p)", self);
    g_return_val_if_fail(RP_IS_NETWORK_ADDRESS_SOCKET_INTERFACE_IMPL_FACTORY(self), NULL);
    return RP_NETWORK_ADDRESS_SOCKET_INTERFACE(
        rp_singleton_manager_get(self->m_singleton_manager,
                                    SINGLETON_MANAGER_REGISTERED_NAME(socket_interface),
                                    singleton_factory_cb,
                                    false));
}
