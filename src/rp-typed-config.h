/*
 * rp-typed-config.h
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Base class for an extension factory.
 */
#define RP_TYPE_UNTYPED_FACTORY rp_untyped_factory_get_type()
G_DECLARE_INTERFACE(RpUntypedFactory, rp_untyped_factory, RP, UNTYPED_FACTORY, GObject)

struct _RpUntypedFactoryInterface {
    GTypeInterface parent_iface;

    const char* (*name)(RpUntypedFactory*);
    const char* (*category)(RpUntypedFactory*);
    //TODO...virtual std::set<std::string> configTypes() { return {}; }
};

static inline const char*
rp_untyped_factory_name(RpUntypedFactory* self)
{
    return RP_IS_UNTYPED_FACTORY(self) ?
        RP_UNTYPED_FACTORY_GET_IFACE(self)->name(self) : NULL;
}
static inline const char*
rp_untyped_factory_category(RpUntypedFactory* self)
{
    return RP_IS_UNTYPED_FACTORY(self) ?
        RP_UNTYPED_FACTORY_GET_IFACE(self)->category(self) : NULL;
}


/**
 * Base class for an extension factory configured by a typed proto message.
 */
#define RP_TYPE_TYPED_FACTORY rp_typed_factory_get_type()
G_DECLARE_INTERFACE(RpTypedFactory, rp_typed_factory, RP, TYPED_FACTORY, RpUntypedFactory)

struct _RpTypedFactoryInterface {
    GTypeInterface parent_iface;

};

G_END_DECLS
