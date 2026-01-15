/*
 * vhost.h
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <glib-object.h>
#include "rproxy.h"

G_BEGIN_DECLS

void vhost_free(vhost_t* self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(vhost_t, vhost_free)
vhost_t* vhost_new(vhost_cfg_t* config);

G_END_DECLS
