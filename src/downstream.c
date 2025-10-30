/*
 * Copyright [2012] [Mandiant, inc]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(downstream_NOISY) || defined(ALL_NOISY)) && !defined(NO_downstream_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-codec-client-prod.h"
#include "rp-net-client-conn-impl.h"
#include "rp-headers.h"
#include "utils/header_value_parser.h"

/**
 * @brief search through a list of downstream_t's and attempt to find one that
 *        has a name that matches the string.
 *
 * @param downstreams
 * @param name
 *
 * @return
 */
downstream_t*
downstream_find_by_name(lztq* downstreams, const char* name)
{
    LOGD("(%p, %p(%s))", downstreams, name, name);

    g_return_val_if_fail(downstreams != NULL, NULL);
    g_return_val_if_fail(name != NULL, NULL);

    for (lztq_elem* elem = lztq_first(downstreams); elem; elem = lztq_next(elem))
    {
        downstream_t* downstream = lztq_elem_data(elem);
        g_assert(downstream != NULL);

        if (!strcmp(downstream->config->name, name))
        {
            LOGD("found %p", downstream);
            return downstream;
        }
    }

    LOGD("not found");
    return NULL;
}

/**
 * @brief attempts to find an idle downstream connection with the most idle connections.
 *
 * @param rule
 *
 * @return a downstream connection, otherwise NULL if no downstreams are avail.
 */
downstream_t*
downstream_get_most_idle(rule_t* rule)
{
    LOGD("(%p)", rule);

    g_assert(rule != NULL);
    g_assert(rule->downstreams != NULL);

    downstream_t* most_idle = NULL;
    for (lztq_elem* elem = lztq_first(rule->downstreams); elem; elem = lztq_next(elem))
    {
        downstream_t* downstream = lztq_elem_data(elem);
        g_assert(downstream != NULL);

        if (!most_idle)
        {
            most_idle = downstream;
            continue;
        }

        /* check to see if the number of idle connections in the current
         * downstream is higher than the saved downstream.
         */
        if (downstream->num_idle > most_idle->num_idle)
        {
            /* this downstream has more idle connections, swap it over to
             * most_idle to use it.
             */
            most_idle = downstream;
        }
    }

    return most_idle;
}

/**
 * @brief Attempts to find an idle downstream connection with the lowest RTT.
 *
 * @param rule
 *
 * @return a downstream connection on success, NULL if no downstreams are
 *         available.
 */
downstream_t*
downstream_get_lowest_rtt(rule_t* rule)
{
    LOGD("(%p)", rule);

    g_assert(rule != NULL);
    g_assert(rule->downstreams != NULL);

    downstream_t* save = NULL;
    for (lztq_elem* elem = lztq_first(rule->downstreams); elem; elem = lztq_next(elem))
    {
        downstream_t* downstream = lztq_elem_data(elem);
        g_assert(downstream != NULL);

        if (!save)
        {
            save = downstream;
            continue;
        }

        if (downstream->m_rtt < save->m_rtt)
        {
            save = downstream;
        }
    }

    return save;
}

/**
 * @brief Attempts to find the first idle downstream connection available.
 *
 * @param rule
 *
 * @return downstream connection, otherwise NULL if none are available.
 */
downstream_t*
downstream_get_none(rule_t* rule)
{
    LOGD("(%p)", rule);

    g_assert(rule != NULL);
    g_assert(rule->downstreams != NULL);

    for (lztq_elem* elem = lztq_first(rule->downstreams); elem; elem = lztq_next(elem))
    {
        downstream_t* downstream = lztq_elem_data(elem);
        g_assert(downstream != NULL);

        if (downstream->num_idle == 0)
        {
            LOGD("continuing");
            continue;
        }

        return downstream;
    }

    return (downstream_t*)lztq_first(rule->downstreams);
}

/**
 * @brief Attempts to find an idle connection using round-robin on configured downstreams.
 *        It should be noted that if only 1 downstream is configured, this will
 *        fallback to using the RTT load-balancing method.
 *
 * @param rule
 *
 * @return a downstream connection, NULL if none are avail.
 */
downstream_t*
downstream_get_rr(rule_t* rule)
{
    LOGD("(%p)", rule);

    g_assert(rule != NULL);
    g_assert(rule->downstreams != NULL);

    if (lztq_size(rule->downstreams) < 2)
    {
        LOGD("getting lowest rtt");
        return (downstream_t*)lztq_elem_data(lztq_first(rule->downstreams));
    }

    lztq_elem* last_used_elem = rule->last_downstream_used;
    LOGD("last_used_elem %p", last_used_elem);

    lztq_elem* downstream_elem;
    if (!last_used_elem)
    {
        downstream_elem = lztq_first(rule->downstreams);
        last_used_elem  = lztq_last(rule->downstreams);
    }
    else
    {
        downstream_elem = lztq_next(last_used_elem);
    }

    if (!downstream_elem)
    {
        LOGD("end of list");
        /* we're at the end of the list, circle back to the first */
        downstream_elem = lztq_first(rule->downstreams);
    }

    downstream_t* downstream = lztq_elem_data(downstream_elem);
    g_assert(downstream != NULL);

    rule->last_downstream_used = downstream_elem;

    return downstream;
}         /* downstream_get_rr */

downstream_t*
downstream_get(rule_t* rule)
{
    LOGD("(%p)", rule);

    g_assert(rule != NULL);

    rule_cfg_t* rcfg = rule->config;
    g_assert(rcfg != NULL);

    switch (rcfg->lb_method)
    {
        case lb_method_rtt:
            LOGD("rtt");
            return downstream_get_lowest_rtt(rule);
        case lb_method_most_idle:
            LOGD("most idle");
            return downstream_get_most_idle(rule);
        case lb_method_rr:
            LOGD("round robbin");
            return downstream_get_rr(rule);
        case lb_method_none:
            LOGD("none");
            return downstream_get_none(rule);
        case lb_method_rand:
        default:
            LOGE("unknown lb method %d", rcfg->lb_method);
            break;
    }

    return NULL;
}

static int
downstream_ssl_init(downstream_t* self, evhtp_ssl_cfg_t* cfg)
{
    static int session_id_context = 1;

    LOGD("(%p, %p)", self, cfg);

    g_return_val_if_fail(self != NULL, -1);
    g_return_val_if_fail(cfg != NULL, -1);
    g_return_val_if_fail(cfg->pemfile != NULL, -1);

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    SSL_library_init();
    ERR_load_crypto_strings();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
#else
    /* unnecessary in OpenSSL 1.1.0 */
    /*
     * if (OPENSSL_init_ssl(OPENSSL_INIT_SSL_DEFAULT, NULL) == 0) {
     *  log_error("OPENSSL_init_ssl");
     *  return -1;
     * }
     *
     * if (OPENSSL_init_crypto(
     *      OPENSSL_INIT_ADD_ALL_CIPHERS |
     *      OPENSSL_INIT_ADD_ALL_DIGESTS |
     *      OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
     *  log_error("OPENSSL_init_crypto");
     *  return -1;
     * }
     */
#endif
    if (RAND_poll() != 1)
    {
        LOGE("RAND_poll");
        return -1;
    }

    unsigned char c;
    if (RAND_bytes(&c, 1) != 1)
    {
        LOGE("RAND_bytes");
        return -1;
    }

#if OPENSSL_VERSION_NUMBER < 0x10000000L
    STACK_OF(SSL_COMP) * comp_methods = SSL_COMP_get_compression_methods();
    sk_SSL_COMP_zero(comp_methods);
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    self->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
#else
    self->ssl_ctx = SSL_CTX_new(TLS_client_method());
#endif

    g_assert(self->ssl_ctx);

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    SSL_CTX_set_options(self->ssl_ctx, SSL_MODE_RELEASE_BUFFERS | SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_timeout(self->ssl_ctx, cfg->ssl_ctx_timeout);
#endif

    SSL_CTX_set_options(self->ssl_ctx, cfg->ssl_opts);

#ifndef OPENSSL_NO_ECDH
    if (cfg->named_curve != NULL)
    {
        EC_KEY * ecdh = NULL;
        int      nid  = 0;

        nid = OBJ_sn2nid(cfg->named_curve);

        if (nid == 0)
        {
            LOGE("ECDH initialization failed: unknown curve %s", cfg->named_curve);
        }

        ecdh = EC_KEY_new_by_curve_name(nid);

        if (ecdh == NULL)
        {
            LOGE("ECDH initialization failed for curve %s", cfg->named_curve);
        }

        SSL_CTX_set_tmp_ecdh(self->ssl_ctx, ecdh);
        EC_KEY_free(ecdh);
    }

#endif      /* OPENSSL_NO_ECDH */
#ifndef OPENSSL_NO_DH
    if (cfg->dhparams != NULL)
    {
        FILE * fh;
        DH   * dh;

        fh = fopen(cfg->dhparams, "r");

        if (fh != NULL)
        {
            dh = PEM_read_DHparams(fh, NULL, NULL, NULL);
            if (dh != NULL)
            {
                SSL_CTX_set_tmp_dh(self->ssl_ctx, dh);
                DH_free(dh);
            }
            else
            {
                LOGE("DH initialization failed: unable to parse file %s", cfg->dhparams);
            }

            fclose(fh);
        }
        else
        {
            LOGE("DH initialization failed: unable to open file %s", cfg->dhparams);
        }
    }

#endif      /* OPENSSL_NO_DH */

    if (cfg->ciphers != NULL)
    {
        if (SSL_CTX_set_cipher_list(self->ssl_ctx, cfg->ciphers) == 0)
        {
            LOGE("set_cipher_list");
            return -1;
        }
    }

    SSL_CTX_load_verify_locations(self->ssl_ctx, cfg->cafile, cfg->capath);
    X509_STORE_set_flags(SSL_CTX_get_cert_store(self->ssl_ctx), cfg->store_flags);
    SSL_CTX_set_verify(self->ssl_ctx, cfg->verify_peer, cfg->x509_verify_cb);

    if (cfg->x509_chk_issued_cb != NULL)
    {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        self->ssl_ctx->cert_store->check_issued = cfg->x509_chk_issued_cb;
#else
        X509_STORE_set_check_issued(SSL_CTX_get_cert_store(self->ssl_ctx), cfg->x509_chk_issued_cb);
#endif
    }

    if (cfg->verify_depth)
    {
        LOGD("setting verify depth %d", cfg->verify_depth);
        SSL_CTX_set_verify_depth(self->ssl_ctx, cfg->verify_depth);
    }

    long cache_mode;
    switch (cfg->scache_type)
    {
        case evhtp_ssl_scache_type_disabled:
            cache_mode = SSL_SESS_CACHE_OFF;
            break;
        default:
            cache_mode = SSL_SESS_CACHE_CLIENT;
            break;
    }         /* switch */

    if (SSL_CTX_use_certificate_chain_file(self->ssl_ctx, cfg->pemfile) != 1)
    {
        int err = errno;
        LOGE("private key file failed %d(%s)", err, g_strerror(err));
        return -1;
    }

    char* const key = cfg->privfile ? cfg->privfile : cfg->pemfile;

    if (cfg->decrypt_cb != NULL)
    {
        EVP_PKEY * pkey = cfg->decrypt_cb(key);

        if (pkey == NULL)
        {
            return -1;
        }

        SSL_CTX_use_PrivateKey(self->ssl_ctx, pkey);

        /*cleanup */
        EVP_PKEY_free(pkey);
    }
    else if (SSL_CTX_use_PrivateKey_file(self->ssl_ctx, key, SSL_FILETYPE_PEM) != 1)
    {
        int err = errno;
        LOGE("private key file failed %d(%s)", err, g_strerror(err));
        return -1;
    }

    SSL_CTX_set_session_id_context(self->ssl_ctx,
                                   (void *)&session_id_context,
                                    sizeof(session_id_context));

    SSL_CTX_set_app_data(self->ssl_ctx, self);
    SSL_CTX_set_session_cache_mode(self->ssl_ctx, cache_mode);

#if 0
    if (cache_mode != SSL_SESS_CACHE_OFF)
    {
        SSL_CTX_sess_set_cache_size(self->ssl_ctx,
            cfg->scache_size ? cfg->scache_size : 1024);

        if (cfg->scache_type == evhtp_ssl_scache_type_builtin ||
            cfg->scache_type == evhtp_ssl_scache_type_user) {
            SSL_CTX_sess_set_new_cb(htp->ssl_ctx, htp__ssl_add_scache_ent_);
            SSL_CTX_sess_set_get_cb(htp->ssl_ctx, htp__ssl_get_scache_ent_);
            SSL_CTX_sess_set_remove_cb(htp->ssl_ctx, htp__ssl_delete_scache_ent_);

            if (cfg->scache_init)
            {
                cfg->args = (cfg->scache_init)(htp);
            }
        }
    }
#endif//0

    return 0;
}         /* downstream_ssl_init */

/**
 * @brief frees a downstream connection resource
 *
 * @param connection
 */
void
downstream_conn_free(downstream_c_t* self)
{
    LOGD("(%p)", self);

    g_return_if_fail(self != NULL);

#ifdef WITH_RETRY_TIMER
    if (self->retry_timer)
    {
        LOGD("freeing retry timer");
        event_free(self->retry_timer);
    }
#endif//WITH_RETRY_TIMER

    g_clear_pointer(&self->ssl, SSL_free);
    g_free(self);
}

/**
 * @brief creates a new downstream connection using downstream_t information
 *
 * Initializes a downstream connection along with creating a retry event timer.
 *
 * @param evbase
 * @param downstream the parent downstream_t instance
 *
 * @return downstream_c_t
 */
downstream_c_t*
downstream_conn_new(evbase_t* evbase, downstream_t* downstream)
{
    LOGD("(%p, %p)", evbase, downstream);

    g_return_val_if_fail(evbase != NULL, NULL);

    downstream_c_t* self = g_new0(downstream_c_t, 1);
    g_assert(self);

    self->parent       = downstream;
    self->rtt          = DBL_MAX;

#ifdef WITH_RETRY_TIMER
    self->retry_timer  = evtimer_new(evbase, downstream_conn_retry, self);
#endif//0

    LOGD("self %p, %zu bytes", self, sizeof(*self));
    return self;
}

/**
 * @brief frees all downstream connections and the parent
 *
 * @param downstream
 */
void
downstream_free(void* arg)
{
    LOGD("(%p)", arg);

    downstream_t* self = arg;
    if (!self)
    {
        LOGD("self is null");
        return;
    }

    downstream_c_t* conn;
    downstream_c_t* save;

    /* free all of the active/idle/down connections */
    for (conn = TAILQ_FIRST(&self->active); conn; conn = save)
    {
        save = TAILQ_NEXT(conn, next);

        downstream_conn_free(conn);
    }

    for (conn = TAILQ_FIRST(&self->idle); conn; conn = save)
    {
        save = TAILQ_NEXT(conn, next);

        downstream_conn_free(conn);
    }

    g_free(self);
}

/**
 * @brief allocates a downstream_t parent and initializes the downstream
 *        connection queues.
 *
 * @param rproxy
 * @param cfg
 *
 * @return downstream_t on success, NULL on error
 */
downstream_t*
downstream_new(rproxy_t* rproxy, downstream_cfg_t* cfg)
{
    LOGD("(%p, %p(%s))", rproxy, cfg, cfg->name);

    g_return_val_if_fail(rproxy != NULL, NULL);
    g_return_val_if_fail(cfg != NULL, NULL);

    downstream_t* self = g_new0(downstream_t, 1);
    if (!self)
    {
        LOGE("alloc failed");
        return NULL;
    }

    if (cfg->ssl_cfg && downstream_ssl_init(self, cfg->ssl_cfg) != 0)
    {
        LOGE("alloc failed");
        downstream_free(self);
        return NULL;
    }

    self->config = cfg;
    self->rproxy = rproxy;

    TAILQ_INIT(&self->active);
    TAILQ_INIT(&self->idle);

    LOGD("self %p, %zu bytes", self, sizeof(*self));
    return self;
}
