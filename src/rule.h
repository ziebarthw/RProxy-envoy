/*
 * rule.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

void rule_free(rule_t* self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(rule_t, rule_free)
rule_t* rule_new(rule_cfg_t* config, vhost_t* vhost);

G_END_DECLS
