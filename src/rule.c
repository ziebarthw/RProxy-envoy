/*
 * rule.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rule_NOISY) || defined(ALL_NOISY)) && !defined(NO_rule_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rule.h"

void
rule_free(rule_t* self)
{
    LOGD("(%p)", self);
    g_return_if_fail(self != NULL);
    g_clear_pointer(&self->upstreams, lztq_free);
    g_free(self);
}

rule_t*
rule_new(rule_cfg_t* config, vhost_t* vhost)
{
    LOGD("(%p, %p)", config, vhost);
    g_return_val_if_fail(config != NULL, NULL);
    g_autoptr(rule_t) self = g_new0(rule_t, 1);
    self->upstreams = lztq_new();
    if (!self->upstreams) return NULL;
    self->config = config;
    self->parent_vhost = vhost;
    NOISY_MSG_("self %p, %zu bytes", self, sizeof(*self));
    return g_steal_pointer(&self);
}
