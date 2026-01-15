/*
 * rp-run-helper.c
 * Copyright (C) 2025 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_run_helper_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_run_helper_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-signal.h"
#include "server/rp-run-helper.h"

struct _RpRunHelper {
    GObject parent_instance;

    RpSignalEventPtr m_sigterm;
    RpSignalEventPtr m_sigint;
    //TODO...RpSignalEventPtr m_sig_user_1;
    RpSignalEventPtr m_sig_hup;
};

G_DEFINE_FINAL_TYPE(RpRunHelper, rp_run_helper, G_TYPE_OBJECT)

static void
sigterm_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpServerInstance* instance = arg;
    rp_server_instance_shutdown(instance);
}

static void
sigint_cb(gpointer arg)
{
    NOISY_MSG_("(%p)", arg);
    RpServerInstance* instance = arg;
    rp_server_instance_shutdown(instance);
}

static void
sighup_cb(gpointer arg G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", arg);
    g_warning("Caught and eating SIGHUP. See documentation for how to hot restart.");
}

OVERRIDE void
dispose(GObject* obj)
{
    NOISY_MSG_("(%p)", obj);

    RpRunHelper* self = RP_RUN_HELPER(obj);
    g_clear_object(&self->m_sigterm);
    g_clear_object(&self->m_sigint);
    g_clear_object(&self->m_sig_hup);

    G_OBJECT_CLASS(rp_run_helper_parent_class)->dispose(obj);
}

static void
rp_run_helper_class_init(RpRunHelperClass* klass)
{
    LOGD("(%p)", klass);

    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = dispose;
}

static void
rp_run_helper_init(RpRunHelper* self G_GNUC_UNUSED)
{
    NOISY_MSG_("(%p)", self);
}

RpRunHelper*
rp_run_helper_new(RpServerInstance* instance, /*TODO...const Options& options,*/ RpDispatcher* dispatcher,
                    RpClusterManager* cm G_GNUC_UNUSED
                    /*TODO..., void(*workers_start_cb)(gpointer), gpointer workers_start_arg*/)
{
    LOGD("(%p, %p, %p)", instance, dispatcher, cm);

    g_return_val_if_fail(RP_IS_SERVER_INSTANCE(instance), NULL);
    g_return_val_if_fail(RP_IS_DISPATCHER(dispatcher), NULL);
    g_return_val_if_fail(RP_IS_CLUSTER_MANAGER(cm), NULL);

    RpRunHelper* self = g_object_new(RP_TYPE_RUN_HELPER, NULL);
    //TODO...if (options.signalHandlingEnabled())
    self->m_sigterm = rp_dispatcher_listen_for_signal(dispatcher, SIGTERM, sigterm_cb, instance);
    self->m_sigint = rp_dispatcher_listen_for_signal(dispatcher, SIGINT, sigint_cb, instance);
    //TODO...sig_user_1_ = dispatcher.listenForSignal(SIGUSR1, [&access_log_manager]() {...reopen access logs});
    self->m_sig_hup = rp_dispatcher_listen_for_signal(dispatcher, SIGHUP, sighup_cb, NULL);

    //TODO...Start overload manager before workers.

    //TODO...cm.setInitializedCb([&instance, &init_manager, &cm, this](){...})

    return self;
}
