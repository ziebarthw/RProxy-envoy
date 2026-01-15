/*
 * vhost.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(vhost_NOISY) || defined(ALL_NOISY)) && !defined(NO_vhost_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "vhost.h"

void
vhost_free(vhost_t* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_free(self);
}

vhost_t*
vhost_new(vhost_cfg_t* config)
{
    LOGD("(%p)", config);
    g_return_val_if_fail(config != NULL, NULL);
    g_autoptr(vhost_t) self = g_new0(vhost_t, 1);
    self->config = config;
    NOISY_MSG_("self %p, %zu bytes", self, sizeof(*self));
    return g_steal_pointer(&self);
}
