/*
 * rp-filter-state-impl.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_filter_state_impl_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_filter_state_impl_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#ifndef OVERRIDE
#define OVERRIDE static
#endif

#include "stream_info/rp-filter-state-impl.h"

struct _RpFilterStateImpl {
    GObject parent_instance;

    SHARED_PTR(RpFilterState) m_ancestor;
    UNIQUE_PTR(RpFilterState) m_parent;
    RpFilterStateLifeSpan_e m_life_span;
//TODO...flat_hash_map<std::string, std::unique_ptr<FilterObject>> data_storage_;
    UNIQUE_PTR(GHashTable) m_data_storage;
};

static void filter_state_iface_init(RpFilterStateInterface* iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(RpFilterStateImpl, rp_filter_state_impl, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(RP_TYPE_FILTER_STATE, filter_state_iface_init)
)

static inline GHashTable*
ensure_data_storage(RpFilterStateImpl* self)
{
    NOISY_MSG_("(%p)", self);
    if (self->m_data_storage)
    {
        NOISY_MSG_("pre-allocated data storage %p", self->m_data_storage);
        return self->m_data_storage;
    }
    self->m_data_storage = g_hash_table_new(g_str_hash, g_str_equal);
    NOISY_MSG_("allocated data storage %p", self->m_data_storage);
    return self->m_data_storage;
}

static void
maybe_create_parent(RpFilterStateImpl* self, RpFilterState* ancestor)
{
    NOISY_MSG_("(%p, %p)", self, ancestor);

    if (self->m_parent || self->m_life_span >= RpFilterStateLifeSpan_TopSpan)
    {
        NOISY_MSG_("parent %p", self->m_parent);
        return;
    }

    RpFilterStateLifeSpan_e parent_life_span = self->m_life_span + 1;

    if (!ancestor || rp_filter_state_life_span(ancestor) < parent_life_span)
    {
NOISY_MSG_("calling rp_filter_state_impl_new(NULL, %d)", parent_life_span);
        self->m_parent = RP_FILTER_STATE(rp_filter_state_impl_new(NULL, parent_life_span));
    }
    else if (rp_filter_state_life_span(ancestor) == parent_life_span)
    {
NOISY_MSG_("ancestor %p(%u)", ancestor, G_OBJECT(ancestor)->ref_count);
        self->m_parent = g_object_ref(ancestor);
    }
    else
    {
NOISY_MSG_("calling rp_filter_state_impl_new(%p, %d)", ancestor, parent_life_span);
        self->m_parent = RP_FILTER_STATE(
            rp_filter_state_impl_new(g_object_take_ref(ancestor), parent_life_span));
    }
}

static bool
has_data_with_name_internally(RpFilterStateImpl* self, const char* data_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, data_name, data_name);
    return self->m_data_storage && g_hash_table_contains(self->m_data_storage, data_name);
}

static bool
has_data_with_name(RpFilterStateImpl* self, const char* data_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, data_name, data_name);
    return has_data_with_name_internally(self, data_name) ||
            (self->m_parent && has_data_with_name(RP_FILTER_STATE_IMPL(self->m_parent), data_name));
}

static RpFilterStateLifeSpan_e
life_span_i(RpFilterState* self)
{
    NOISY_MSG_("(%p)", self);
    return RP_FILTER_STATE_IMPL(self)->m_life_span;
}

static void
set_data_i(RpFilterState* self, const char* data_name, gpointer data, RpFilterStateStateType_e state_type, RpFilterStateLifeSpan_e life_span)
{
    NOISY_MSG_("(%p, %p(%s), %p, %d, %d)", self, data_name, data_name, data, state_type, life_span);

    RpFilterStateImpl* me = RP_FILTER_STATE_IMPL(self);
NOISY_MSG_("life span %d(%d)", life_span, me->m_life_span);
    if (life_span > me->m_life_span)
    {
        if (has_data_with_name_internally(me, data_name))
        {
            NOISY_MSG_("FilterStateAccessViolation: FilterState::setData called twice with conflicting lifespan on the data_name.");
            return;
        }
        maybe_create_parent(me, NULL);
        rp_filter_state_set_data(me->m_parent, data_name, data, state_type, life_span);
        return;
    }
    if (me->m_parent && has_data_with_name(RP_FILTER_STATE_IMPL(me->m_parent), data_name))
    {
        NOISY_MSG_("FilterStateAccessViolation: FilterState::setData called twice with conflicting life_span on the same data_name.");
        return;
    }
    //TODO...const auto& it = data_storate_.find(data_name);
    g_hash_table_insert(ensure_data_storage(me), (gpointer)data_name, data);
}

static gpointer
get_data_i(RpFilterState* self, const char* data_name)
{
    NOISY_MSG_("(%p, %p(%s))", self, data_name, data_name);
    RpFilterStateImpl* me = RP_FILTER_STATE_IMPL(self);
    gpointer it = me->m_data_storage ?
                    g_hash_table_lookup(me->m_data_storage, data_name) : NULL;
    if (!it)
    {
        if (me->m_parent)
        {
            NOISY_MSG_("checking parent %p", me->m_parent);
            return rp_filter_state_get_data(me->m_parent, data_name);
        }
        return NULL;
    }
    return it;
}

static void
filter_state_iface_init(RpFilterStateInterface* iface)
{
    LOGD("(%p)", iface);
    iface->life_span = life_span_i;
    iface->set_data = set_data_i;
    iface->get_data = get_data_i;
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpFilterStateImpl* self = RP_FILTER_STATE_IMPL(obj);
    g_clear_object(&self->m_parent);
    g_clear_pointer(&self->m_data_storage, g_hash_table_unref);

    G_OBJECT_CLASS(rp_filter_state_impl_parent_class)->dispose(obj);
}

static void
rp_filter_state_impl_class_init(RpFilterStateImplClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_filter_state_impl_init(RpFilterStateImpl* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

static inline RpFilterStateImpl*
constructed(RpFilterStateImpl* self)
{
    NOISY_MSG_("(%p)", self);

    if (self->m_ancestor)
    {
        NOISY_MSG_("calling maybe_create_parent(%p, %p)", self, self->m_ancestor);
        maybe_create_parent(self, g_steal_pointer(&self->m_ancestor));
    }
    return self;
}

RpFilterStateImpl*
rp_filter_state_impl_new(RpFilterState* ancestor, RpFilterStateLifeSpan_e life_span)
{
    LOGD("(%p, %d)", ancestor, life_span);
    RpFilterStateImpl* self = g_object_new(RP_TYPE_FILTER_STATE_IMPL, NULL);
    self->m_ancestor = ancestor;
    self->m_life_span = life_span;
    return constructed(self);
}
