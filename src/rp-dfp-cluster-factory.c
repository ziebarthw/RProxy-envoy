/*
 * rp-dfp-cluster-factory.c
 */

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(rp_dfp_cluster_factory_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_dfp_cluster_factory_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-dfp-cluster-impl.h"
#include "rp-dfp-cluster-factory.h"

struct _RpDfpClusterFactory {
    RpClusterFactoryImplBase parent_instance;
};

G_DEFINE_FINAL_TYPE(RpDfpClusterFactory, rp_dfp_cluster_factory, RP_TYPE_CLUSTER_FACTORY_IMPL_BASE)

OVERRIDE RpClusterImplBase*
create_cluster_impl(RpClusterFactoryImplBase* self, RpClusterCfg* cluster_config, RpClusterFactoryContext* context)
{
    NOISY_MSG_("(%p, %p, %p)", self, cluster_config, context);
    return RP_CLUSTER_IMPL_BASE(rp_dfp_cluster_impl_create_cluster_with_config(cluster_config, context));
}

static void
cluster_factory_impl_base_class_init(RpClusterFactoryImplBaseClass* klass)
{
    LOGD("(%p)", klass);
    klass->create_cluster_impl = create_cluster_impl;
}

static void
rp_dfp_cluster_factory_class_init(RpDfpClusterFactoryClass* klass)
{
    LOGD("(%p)", klass);
    cluster_factory_impl_base_class_init(RP_CLUSTER_FACTORY_IMPL_BASE_CLASS(klass));
}

static void
rp_dfp_cluster_factory_init(RpDfpClusterFactory* self G_GNUC_UNUSED)
{
    LOGD("(%p)", self);
}

RpDfpClusterFactory*
rp_dfp_cluster_factory_new(void)
{
    LOGD("()");
    return g_object_new(RP_TYPE_DFP_CLUSTER_FACTORY, NULL);
}
