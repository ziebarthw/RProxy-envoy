/* Copyright [2012] [Mandiant, inc]
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef USE_MALLOPT
#include <malloc.h>
#endif

#ifndef NO_RLIMIT
#include <sys/resource.h>
#endif

#include "rproxy.h"

static void rproxy_process_pending(int, short, void *);

int
append_ssl_x_headers(headers_cfg_t * headers_cfg, evhtp_request_t * upstream_req) {
    evhtp_headers_t * headers;
    x509_ext_cfg_t  * x509_ext_cfg;

    evhtp_ssl_t     * ssl;

    if (!headers_cfg || !upstream_req) {
        return -1;
    }

    if (!(ssl = upstream_req->conn->ssl)) {
        return 0;
    }

    if (!(headers = upstream_req->headers_in)) {
        return -1;
    }

    /* remove all headers the client might have sent, if any of these
     * configurations are set to false, this allows the client to send
     * their own versions. This is potentially dangerous, so we remove them
     */

    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Subject"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Issuer"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Notbefore"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Notafter"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Serial"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Cipher"));
    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-SSL-Certificate"));

    if (headers_cfg->x_ssl_subject == true) {
        unsigned char * subj_str;

        if ((subj_str = ssl_subject_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Subject", subj_str, 0, 1));

            free(subj_str);
        }
    }

    if (headers_cfg->x_ssl_issuer == true) {
        unsigned char * issr_str;

        if ((issr_str = ssl_issuer_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Issuer", issr_str, 0, 1));

            free(issr_str);
        }
    }

    if (headers_cfg->x_ssl_notbefore == true) {
        unsigned char * nbf_str;

        if ((nbf_str = ssl_notbefore_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Notbefore", nbf_str, 0, 1));

            free(nbf_str);
        }
    }

    if (headers_cfg->x_ssl_notafter == true) {
        unsigned char * naf_str;

        if ((naf_str = ssl_notafter_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Notafter", naf_str, 0, 1));

            free(naf_str);
        }
    }

    if (headers_cfg->x_ssl_serial == true) {
        unsigned char * ser_str;

        if ((ser_str = ssl_serial_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Serial", ser_str, 0, 1));

            free(ser_str);
        }
    }

    if (headers_cfg->x_ssl_cipher == true) {
        unsigned char * cip_str;

        if ((cip_str = ssl_cipher_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Cipher", cip_str, 0, 1));

            free(cip_str);
        }
    }

    if (headers_cfg->x_ssl_sha1 == true) {
        unsigned char * sha1_str;

        if ((sha1_str = ssl_sha1_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Sha1", sha1_str, 0, 1));

            free(sha1_str);
        }
    }

    if (headers_cfg->x_ssl_certificate == true) {
        unsigned char * cert_str;

        if ((cert_str = ssl_cert_tostr(ssl))) {
            evhtp_headers_add_header(headers,
                                     evhtp_header_new("X-SSL-Certificate", cert_str, 0, 1));

            free(cert_str);
        }
    }

    {
        lztq_elem * x509_elem;
        lztq_elem * x509_save;

        for (x509_elem = lztq_first(headers_cfg->x509_exts); x509_elem; x509_elem = x509_save) {
            unsigned char * ext_str;

            x509_ext_cfg = lztq_elem_data(x509_elem);
            assert(x509_ext_cfg != NULL);

            if ((ext_str = ssl_x509_ext_tostr(ssl, x509_ext_cfg->oid))) {
                evhtp_headers_add_header(headers,
                                         evhtp_header_new(x509_ext_cfg->name, ext_str, 0, 1));
                free(ext_str);
            }

            x509_save = lztq_next(x509_elem);
        }
    }

    return 0;
} /* append_ssl_x_headers */

int
append_x_headers(headers_cfg_t * headers_cfg, evhtp_request_t * upstream_req) {
    evhtp_headers_t * headers;
    char              tmp1[1024];
    char              tmp2[1024];

    if (!headers_cfg || !upstream_req) {
        return -1;
    }

    if (!(headers = upstream_req->headers_in)) {
        return -1;
    }

    if (headers_cfg->x_forwarded_for == true) {
        struct sockaddr * sa;
        void            * src;
        char            * fmt;
        unsigned short    port;
        int               sres;

        src = NULL;
        sa  = upstream_req->conn->saddr;

        if (sa->sa_family == AF_INET) {
            src  = &(((struct sockaddr_in *)sa)->sin_addr);
            port = ntohs(((struct sockaddr_in *)sa)->sin_port);
            fmt  = "%s:%hu";
        } else if (sa->sa_family == AF_INET6) {
            src  = &(((struct sockaddr_in6 *)sa)->sin6_addr);
            port = ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
            fmt  = "[%s]:%hu";
        }

        if (!src || !evutil_inet_ntop(sa->sa_family, src, tmp1, sizeof(tmp1))) {
            return -1;
        }

        sres = snprintf(tmp2, sizeof(tmp2), fmt, tmp1, port);

        if (sres < 0 || sres >= sizeof(tmp2)) {
            return -1;
        }

        evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, "X-Forwarded-For"));

        evhtp_headers_add_header(headers,
                                 evhtp_header_new("X-Forwarded-For", tmp2, 0, 1));
    }

    if (upstream_req->conn->ssl) {
        if (append_ssl_x_headers(headers_cfg, upstream_req) < 0) {
            return -1;
        }
    }

    return 0;
} /* append_x_headers */

evhtp_res
upstream_on_write(evhtp_connection_t * conn, void * args) {
    request_t * ds_req;

    ds_req = args;
    assert(ds_req != NULL);

    if (ds_req->hit_upstream_highwm) {
        /* upstream hit a high watermark for its write buffer, but now the
         * buffer has been fully flushed, so we can enable the read side of the
         * downstream again.
         */
        bufferevent_enable(ds_req->downstream_conn->connection, EV_READ);
        ds_req->hit_upstream_highwm = 0;
    }

    return EVHTP_RES_OK;
}

/**
 * @brief data was read from an upstream connection, pass it down to the
 * downstream.
 *
 * @param bev
 * @param arg
 */
void
passthrough_readcb(evbev_t * bev, void * arg) {
    request_t * request;

    request = arg;
    assert(request != NULL);

    /* write the data read from the upstream bufferevent to the downstream
     * bufferevent.
     */
    bufferevent_write_buffer(request->downstream_bev, bufferevent_get_input(bev));
}

void
passthrough_writecb(evbev_t * bev, void * arg) {
    return;
}

void
passthrough_eventcb(evbev_t * bev, short events, void * arg) {
    int              res;
    request_t      * request;
    downstream_c_t * ds_conn;


    request = arg;
    assert(request != NULL);

    ds_conn = request->downstream_conn;

    bufferevent_free(request->upstream_bev);
    downstream_connection_set_down(ds_conn);

    request_free(request);

    ds_conn->request = NULL;
}

evhtp_res
send_upstream_headers(evhtp_request_t * upstream_req, evhtp_headers_t * hdrs, void * arg) {
    /* evhtp has parsed the inital request (request + headers). From this
     * request generate a proper request which can be pipelined to the
     * downstream connection
     */
    request_t      * request;
    rproxy_t       * rproxy;
    evhtp_header_t * connection_hdr;
    evbuf_t        * buf;
    rule_cfg_t     * rule_cfg;
    rule_t         * rule;
    headers_cfg_t  * headers;
    char           * query_args;
    evhtp_res        res = EVHTP_RES_OK;

    request  = arg;
    assert(request != NULL);

    rproxy   = request->rproxy;
    assert(rproxy != NULL);

    rule     = request->rule;
    assert(rule != NULL);

    rule_cfg = rule->config;
    assert(rule_cfg != NULL);

    if (request->pending == 1) {
        abort();
    }

    if (!request->downstream_conn->connection) {
        logger_log_request_error(rule->err_log, request, "%s(): conn == NULL", __FUNCTION__);
        return EVHTP_RES_ERROR;
    }

    /* default the x-header configuration to the rule, but if not found,
     * fallback to the vhost parent's config.
     */
    headers = rule->config->headers;

    if (!headers) {
        headers = rule->parent_vhost->config->headers;
    }

    /* Add X-Headers to the request if applicable */
    if (headers) {
        if (append_x_headers(headers, upstream_req) < 0) {
            logger_log_request_error(rule->err_log, request,
                                     "%s(): append_x_headers < 0", __FUNCTION__);
            return EVHTP_RES_ERROR;
        }
    }

    /* checks to determine of the upstream request is set to a
     * non keep-alive state, and if it is it magically converts
     * it to a keep-alive request to send to the downstream so
     * the connection remains open
     */
    switch (upstream_req->proto) {
        case EVHTP_PROTO_10:
            if (upstream_req->keepalive > 0) {
                break;
            }

            /* upstream request is HTTP/1.0 with no keep-alive header,
             * to keep our connection alive to the downstream we insert
             * a Connection: Keep-Alive header
             */

            if ((connection_hdr = evhtp_headers_find_header(hdrs, "Connection"))) {
                evhtp_header_rm_and_free(hdrs, connection_hdr);
            }

            connection_hdr = evhtp_header_new("Connection", "Keep-Alive", 0, 0);

            if (connection_hdr == NULL) {
                logger_log_request_error(rule->err_log, request,
                                         "%s(): couldn't create new header %s",
                                         __FUNCTION__, strerror(errno));
                exit(EXIT_FAILURE);
            }

            evhtp_headers_add_header(hdrs, connection_hdr);
            break;
        case EVHTP_PROTO_11:
            if (upstream_req->keepalive > 0) {
                break;
            }

            /* upstream request is HTTP/1.1 but the Connection: close header was
             * present, so just remove the header to keep downstream connection
             * alive.
             */

            const char * v = evhtp_kv_find(hdrs, "Connection");

            connection_hdr = evhtp_headers_find_header(hdrs, "Connection");

            if (!strcasecmp(v, "close")) {
                upstream_req->keepalive = 0;
            }

            evhtp_header_rm_and_free(hdrs, connection_hdr);
            break;
        default:
            logger_log_request_error(rule->err_log, request,
                                     "%s(): unknown proto %d",
                                     __FUNCTION__, upstream_req->proto);
            return EVHTP_RES_ERROR;
    } /* switch */

    buf = util_request_to_evbuffer(upstream_req);
    assert(buf != NULL);

    bufferevent_write_buffer(request->downstream_bev, buf);
    evbuffer_free(buf);

    /* TODO: if the upstream's input bev still has data, write it here... */
    if (rule_cfg->passthrough == true) {
        /* rule configured to be passthrough, so we take ownership of the
         * bufferevent and ignore any state based processing of the request.
         * This allows for non-http specific protocols to flow such as
         * websocket.
         */
        evbev_t * bev;

        bev = evhtp_connection_take_ownership(evhtp_request_get_connection(upstream_req));
        assert(bev != NULL);

        /* our on_headers_complete hook will call this function before the last
         * \n in the \r\n\r\n part, so we have to remove this.
         */
        evbuffer_drain(bufferevent_get_input(bev), 1);

        bufferevent_enable(bev, EV_READ | EV_WRITE);
        bufferevent_setcb(bev,
                          passthrough_readcb,
                          passthrough_writecb,
                          passthrough_eventcb, request);

        request->upstream_request = NULL;

        return EVHTP_RES_USER;
    }



    return res;
} /* send_upstream_headers */

evhtp_res
send_upstream_body(evhtp_request_t * upstream_req, evbuf_t * buf, void * arg) {
    /* stream upstream request body to the downstream server */
    request_t      * request;
    rule_t         * rule;
    rproxy_t       * rproxy;
    downstream_c_t * ds_conn;

    request = arg;
    assert(request != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    rule    = request->rule;
    assert(rule != NULL);

    if (!upstream_req || !buf) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): up_req = %p, buf = %p", __FUNCTION__, upstream_req, buf);
        return EVHTP_RES_FATAL;
    }

    if (!(ds_conn = request->downstream_conn)) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): downstream_conn == NULL", __FUNCTION__);
        return EVHTP_RES_FATAL;
    }

    if (!ds_conn->connection || request->error > 0) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): conn = %p, req error = %d",
                                 __FUNCTION__, ds_conn->connection, request->error);
        evbuffer_drain(buf, -1);

        return EVHTP_RES_ERROR;
    }

    bufferevent_write_buffer(ds_conn->connection, buf);

    if (ds_conn->parent->config->high_watermark > 0) {
        if (evbuffer_get_length(bufferevent_get_output(ds_conn->connection)) >= ds_conn->parent->config->high_watermark) {
            request->hit_highwm = 1;
#ifdef RPROXY_DEBUG
            printf("Hit high-watermark %zu: %zu in output\n",
                   ds_conn->parent->config->high_watermark,
                   evbuffer_get_length(bufferevent_get_output(ds_conn->connection)));
#endif
            evhtp_request_pause(upstream_req);
            return EVHTP_RES_PAUSE;
        }
    }

    return EVHTP_RES_OK;
} /* send_upstream_body */

evhtp_res
send_upstream_new_chunk(evhtp_request_t * upstream_req, uint64_t len, void * arg) {
    request_t      * request;
    rproxy_t       * rproxy;
    downstream_c_t * ds_conn;
    rule_t         * rule;

    request = arg;
    assert(request != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    rule    = request->rule;
    assert(rproxy != NULL);

    if (!upstream_req) {
        logger_log(rproxy->err_log, lzlog_err, "%s(): !upstream_req", __FUNCTION__);

        return EVHTP_RES_FATAL;
    }

    if (!(ds_conn = request->downstream_conn)) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): downstream_conn == NULL", __FUNCTION__);
        return EVHTP_RES_FATAL;
    }

    if (!ds_conn->connection || request->error > 0) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): conn = %p, err = %d",
                                 __FUNCTION__, ds_conn->connection, request->error);
        return EVHTP_RES_ERROR;
    }

    evbuffer_add_printf(bufferevent_get_output(ds_conn->connection),
                        "%x\r\n", (unsigned int)len);

    return EVHTP_RES_OK;
} /* send_upstream_new_chunk */

evhtp_res
send_upstream_chunk_done(evhtp_request_t * upstream_req, void * arg) {
    request_t      * request;
    rproxy_t       * rproxy;
    downstream_c_t * ds_conn;
    rule_t         * rule;

    request = arg;
    assert(request != NULL);

    rule    = request->rule;
    assert(rule != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    ds_conn = request->downstream_conn;
    assert(ds_conn != NULL);

    if (!ds_conn->connection || request->error > 0) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): conn = %p, err = %d",
                                 __FUNCTION__, ds_conn->connection, request->error);
        return EVHTP_RES_ERROR;
    }

    bufferevent_write(ds_conn->connection, "\r\n", 2);
    return EVHTP_RES_OK;
}

evhtp_res
send_upstream_chunks_done(evhtp_request_t * upstream_req, void * arg) {
    request_t      * request;
    rproxy_t       * rproxy;
    rule_t         * rule;
    downstream_c_t * ds_conn;

    request = arg;
    assert(request != NULL);

    rule    = request->rule;
    assert(rule != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    ds_conn = request->downstream_conn;
    assert(ds_conn != NULL);

    if (!ds_conn->connection || request->error > 0) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): dsconn = %p, err = %d", __FUNCTION__,
                                 ds_conn->connection, request->error);
        return EVHTP_RES_ERROR;
    }

    bufferevent_write(ds_conn->connection, "0\r\n\r\n", 5);
    return EVHTP_RES_OK;
}

evhtp_res
upstream_fini(evhtp_request_t * upstream_req, void * arg) {
    request_t      * request;
    rproxy_t       * rproxy;
    downstream_c_t * ds_conn;
    downstream_t   * downstream;
    rule_t         * rule;
    int              res;

    request = arg;
    assert(request != NULL);

    rule    = request->rule;
    assert(rule != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    /* if this downstream request is still pending, remove it from the queue */
    if (request->pending) {
        TAILQ_REMOVE(&rproxy->pending, request, next);
        rproxy->n_pending -= 1;
        request_free(request);
        return EVHTP_RES_OK;
    }

    ds_conn = request->downstream_conn;
    assert(ds_conn != NULL);

    if (REQUEST_HAS_ERROR(request)) {
        logger_log_request_error(rule->err_log, request,
                                 "%s(): we should never get here!", __FUNCTION__);
        downstream_connection_set_down(ds_conn);
    } else {
        downstream_connection_set_idle(ds_conn);
    }

    request_free(request);

    return EVHTP_RES_OK;
} /* upstream_fini */

/**
 * @brief called when an upstream socket encounters an error.
 *
 * @param upstream_req
 * @param arg
 */
static void
upstream_error(evhtp_request_t * upstream_req, short events, void * arg) {
    request_t      * request;
    rproxy_t       * rproxy;
    downstream_c_t * ds_conn;
    rule_t         * rule;

    request = arg;
    assert(request != NULL);

    rule    = request->rule;
    assert(rule != NULL);

    rproxy  = request->rproxy;
    assert(rproxy != NULL);

    evhtp_unset_all_hooks(&upstream_req->hooks);

    logger_log(rule->err_log, lzlog_warn, "%s(): client aborted, err = %x",
               __FUNCTION__, events);

    if (request->pending) {
        /* upstream encountered socket error while still in a pending state */
        assert(request->downstream_conn == NULL);

        TAILQ_REMOVE(&rproxy->pending, request, next);
        rproxy->n_pending -= 1;
        request_free(request);
        return;
    }

    request->upstream_err = 1;

    ds_conn = request->downstream_conn;
    assert(ds_conn != NULL);

    if (!request->reading) {
        /* since we are not currently dealing with data being parsed by
         * downstream_connection_readcb, we must do all the resource cleanup
         * here.
         */

        if (request->done) {
            /* the request was completely finished, so we can safely set the
             * downstream as idle.
             */
            logger_log_request_error(rule->err_log, request,
                                     "%s(): req completed, client aborted", __FUNCTION__);
            downstream_connection_set_idle(ds_conn);
        } else {
            /* request never completed, set the connection to down */
            logger_log_request_error(rule->err_log, request,
                                     "req incomplete, client aborted", __FUNCTION__);
            downstream_connection_set_down(ds_conn);
        }

        if (ds_conn->request == request) {
            ds_conn->request = NULL;
        }

        request_free(request);
    }
} /* upstream_error */

/**
 * @brief allocates a new downstream_t, and appends it to the
 *        rproxy->downstreams list. This is callback for the
 *        lztq_for_each function from rproxy_thread_init().
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static int
add_downstream(lztq_elem * elem, void * arg) {
    rproxy_t         * rproxy = arg;
    downstream_cfg_t * ds_cfg = lztq_elem_data(elem);
    downstream_t     * downstream;
    lztq_elem        * nelem;

    assert(rproxy != NULL);
    assert(ds_cfg != NULL);

    downstream = downstream_new(rproxy, ds_cfg);
    assert(downstream != NULL);

    nelem      = lztq_append(rproxy->downstreams, downstream,
                             sizeof(downstream), downstream_free);
    assert(nelem != NULL);

    return 0;
}

/**
 * @brief creates n connections to the server information contained in a
 *        downstream_t instance. This is the callback for the lztq_for_each
 *        function from rproxy_thread_init() (after the downstream list has been
 *        created.
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static int
start_downstream(lztq_elem * elem, void * arg) {
    evbase_t     * evbase     = arg;
    downstream_t * downstream = lztq_elem_data(elem);

    assert(evbase != NULL);
    assert(downstream != NULL);

    return downstream_connection_init(evbase, downstream);
}

/**
 * @brief match up names in the list of downstream_cfg_t's in rule_cfg->downstreams
 *        to the downstream_t's in the rproxy->downstreams list. If found,
 *        create a rule_t and appends it to the rproxy->rules list.
 *
 * @param elem a lztq elem with the type of vhost_cfg_t *
 * @param arg  the vhost_t *
 *
 * @return
 */
static int
map_vhost_rules_to_downstreams(lztq_elem * elem, void * arg) {
    vhost_t    * vhost = arg;
    rproxy_t   * rproxy;
    rule_cfg_t * rule_cfg;
    lztq_elem  * name_elem;
    lztq_elem  * name_elem_temp;
    rule_t     * rule;

    vhost              = arg;
    assert(arg != NULL);

    rproxy             = vhost->rproxy;
    assert(rproxy != NULL);

    rule_cfg           = lztq_elem_data(elem);
    assert(rule_cfg != NULL);

    rule               = calloc(sizeof(rule_t), 1);
    assert(rule != NULL);

    rule->rproxy       = rproxy;
    rule->config       = rule_cfg;
    rule->parent_vhost = vhost;

    /*
     * if a rule specific logging is found then all is good to go. otherwise
     * if a vhost specific logging is found then set it to rule. otherwise
     * if a server specific logging is found, set both vhost and rule to this.
     */

    if (rule_cfg->req_log) {
        rule->req_log = logger_init(rule_cfg->req_log, 0);
    } else if (vhost->req_log) {
        rule->req_log = vhost->req_log;
    } else {
        rule->req_log  = rproxy->req_log;
        vhost->req_log = rproxy->req_log;
    }

    /*
     * the same logic applies as above for error logging.
     */

    if (rule_cfg->err_log) {
        rule->err_log = logger_init(rule_cfg->err_log, 0);
    } else if (vhost->err_log) {
        rule->err_log = vhost->err_log;
    } else {
        rule->err_log  = rproxy->err_log;
        vhost->err_log = rproxy->err_log;
    }

    rule->downstreams = lztq_new();
    assert(rule->downstreams != NULL);

    /* for each string in the rule_cfg's downstreams section, find the matching
     * downstream_t and append it.
     */
    for (name_elem = lztq_first(rule_cfg->downstreams); name_elem != NULL; name_elem = name_elem_temp) {
        const char   * ds_name;
        downstream_t * ds;
        lztq_elem    * nelem;

        ds_name = lztq_elem_data(name_elem);
        assert(ds_name != NULL);

        if (!(ds = downstream_find_by_name(rproxy->downstreams, ds_name))) {
            /* could not find a downstream_t which has this name! */
            fprintf(stderr, "Could not find downstream named '%s!\n", ds_name);
            exit(EXIT_FAILURE);
        }

        nelem          = lztq_append(rule->downstreams, ds, sizeof(ds), NULL);
        assert(nelem != NULL);

        name_elem_temp = lztq_next(name_elem);
    }


    /* upstream_request_start is passed only the rule_cfg as an argument. this
     * function will call file_rule_from_cfg to get the actual rule from the
     * global rproxy->rules list. Since we compare pointers, it is safe to keep
     * this as one single list.
     */
    lztq_append(rproxy->rules, rule, sizeof(rule), NULL);

    return 0;
} /* map_vhost_rules_to_downstreams */

static rule_t *
find_rule_from_cfg(rule_cfg_t * rule_cfg, lztq * rules) {
    lztq_elem * rule_elem;
    lztq_elem * rule_elem_temp;

    for (rule_elem = lztq_first(rules); rule_elem != NULL; rule_elem = rule_elem_temp) {
        rule_t * rule = lztq_elem_data(rule_elem);

        if (rule->config == rule_cfg) {
            return rule;
        }

        rule_elem_temp = lztq_next(rule_elem);
    }

    return NULL;
}

/**
 * @brief Called when an upstream request is in the pending queue and the
 *        configured timeout has been reached.
 *
 * @param fd
 * @param what
 * @param arg
 */
static void
downstream_pending_timeout(evutil_socket_t fd, short what, void * arg) {
    request_t       * ds_req;
    rproxy_t        * rproxy;
    evhtp_request_t * up_req;
    rule_t          * rule;
    rule_cfg_t      * rule_cfg;

    ds_req   = arg;
    assert(ds_req != NULL);

    rule     = ds_req->rule;
    assert(rule != NULL);

    rule_cfg = rule->config;
    assert(rule_cfg != NULL);

    rproxy   = ds_req->rproxy;
    assert(rproxy != NULL);

    if (rule_cfg->passthrough == true) {
        /*
         * since we're in passthrough mode, we don't need to free any evhtp
         * resources; so we just close the upstream socket and free the request
         */
        if (ds_req->pending) {
            TAILQ_REMOVE(&rproxy->pending, ds_req, next);
            rproxy->n_pending -= 1;
        }

        bufferevent_free(ds_req->upstream_bev);

        logger_log(rule->err_log, lzlog_notice,
                   "%s(): pending timeout hit for upstream (passthrough)", __FUNCTION__);

        return request_free(ds_req);
    }

    up_req = ds_req->upstream_request;
    assert(up_req != NULL);

    /* unset all hooks except for the fini, evhtp_send_reply() will call the
     * fini function after the 503 message has been delivered */
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_headers);
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_new_chunk);
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_chunk_complete);
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_chunks_complete);
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_read);
    evhtp_unset_hook(&up_req->hooks, evhtp_hook_on_error);

    up_req->keepalive = 0;

    evhtp_headers_add_header(up_req->headers_out, evhtp_header_new("Connection", "close", 0, 0));
    evhtp_request_resume(up_req);
    evhtp_send_reply(up_req, 503);

    if (ds_req->rule) {
        logger_log(rule->err_log, lzlog_notice,
                   "%s(): pending timeout hit for upstream client", __FUNCTION__);
    }
} /* downstream_pending_timeout */

/**
 * @brief Before accepting an upstream connection, evhtp will call this function
 *        which will check whether we have hit our max-pending limits, and if so,
 *        inform evhtp to not accept().
 *
 * @param up_conn
 * @param arg
 *
 * @return
 */
evhtp_res
upstream_pre_accept(evhtp_connection_t * up_conn, void * arg) {
    rproxy_t * rproxy;

    if (!(rproxy = evthr_get_aux(up_conn->thread))) {
        return EVHTP_RES_FATAL;
    }

    if (rproxy->server_cfg->max_pending <= 0) {
        /* configured with unlimited pending */
        return EVHTP_RES_OK;
    }

    /* check to see if we have too many pending requests, and if so, drop this
     * connection.
     */
    if ((rproxy->n_pending + 1) > rproxy->server_cfg->max_pending) {
#ifdef RPROXY_DEBUG
        printf("Dropped connection, too many pending requests\n");
#endif
        return EVHTP_RES_ERROR;
    }

    if (rproxy->server_cfg->disable_client_nagle == 1) {
        /* config has requested that client sockets have the nagle algorithm
         * turned off.
         */

        setsockopt(up_conn->sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
    }


    return EVHTP_RES_OK;
}

static evhtp_res
upstream_dump_request(evhtp_request_t * r, const char * host, void * arg) {
    evhtp_send_reply(r, EVHTP_RES_NOTFOUND);

    return EVHTP_RES_ERROR;
}

evhtp_res
upstream_request_start(evhtp_request_t * up_req, const char * hostname, void * arg) {
    /* This function is called whenever evhtp has matched a hostname on a request.
     *
     * Once the downstream request has been initialized and setup, this function
     * will *NOT* immediately start processing the request. This is because a
     * downstream connection may not be available at the time this function is
     * called.
     *
     * Instead, the request is placed in a pending request queue where this
     * queue is processed once downstream_connection_set_idle() has signaled the
     * process_pending event handler that a downstream connection is available.
     *
     * The return value of this function, EVHTP_RES_PAUSE, informs the evhtp
     * backend to suspend reading on the socket on the upstream until
     * process_pending successfully finds an idle downstream connection. From
     * there the upstream request is resumed.
     *
     * TL;DNR: upstream request is not immediately processed, but placed in a
     *         pending queue until a downstream connection becomes available.
     */
    rule_cfg_t         * rule_cfg   = NULL;
    rule_t             * rule       = NULL;
    rproxy_t           * rproxy     = NULL;
    request_t          * ds_req     = NULL;
    server_cfg_t       * serv_cfg   = NULL;
    vhost_cfg_t        * vhost_cfg  = NULL;
    evhtp_connection_t * up_conn    = NULL;
    struct timeval     * pending_tv = NULL;

    /* the rproxy structure is contained within the evthr's aux var */
    if (!(rproxy = evthr_get_aux(up_req->conn->thread))) {
        return EVHTP_RES_FATAL;
    }

    if (!(rule_cfg = arg)) {
        return EVHTP_RES_FATAL;
    }

    if (!(rule = find_rule_from_cfg(rule_cfg, rproxy->rules))) {
        return EVHTP_RES_FATAL;
    }

    if (lztq_size(rule_cfg->downstreams) == 0) {
        /* no downstreams configured for this request, so we 404 this request */
        logger_log(rule->err_log, lzlog_err,
                   "no downstreams configured for rule %s (query %s)",
                   rule_cfg->name, up_req->uri->path->full);
        evhtp_send_reply(up_req, EVHTP_RES_NOTFOUND);
        return EVHTP_RES_ERROR;
    }


    serv_cfg = rproxy->server_cfg;
    assert(serv_cfg != NULL);

    ds_req   = request_new(rproxy);
    assert(ds_req != NULL);

    up_conn  = evhtp_request_get_connection(up_req);
    assert(up_conn != NULL);

    ds_req->upstream_bev     = evhtp_request_get_bev(up_req);
    ds_req->downstream_bev   = NULL;
    ds_req->upstream_request = up_req;
    ds_req->rule = rule;
    ds_req->pending          = 1;
    rproxy->n_pending       += 1;

    /* if a rule has an upstream-[read|write]-timeout config set, we will set a
     * upstream connection-specific timeout that overrides the global one.
     */
    if (rule_cfg->has_up_read_timeout || rule_cfg->has_up_write_timeout) {
        evhtp_connection_set_timeouts(up_conn,
                                      &rule_cfg->up_read_timeout,
                                      &rule_cfg->up_write_timeout);
    }

    if (serv_cfg->pending_timeout.tv_sec || serv_cfg->pending_timeout.tv_usec) {
        /* a pending timeout has been configured, so set an evtimer to trigger
         * if this upstream request remains in the pending queue for that amount
         * of time.
         */
        ds_req->pending_ev = evtimer_new(rproxy->evbase, downstream_pending_timeout, ds_req);
        assert(ds_req->pending_ev != NULL);

        evtimer_add(ds_req->pending_ev, &serv_cfg->pending_timeout);
    }

    /* Since this is called after a host header match, we set upstream request
     * specific evhtp hooks specific to this request. This is done in order
     * to stream the upstream data directly to the downstream and allow for
     * the modification of the request made to the downstream.
     */

    /* call this function once all headers from the upstream have been parsed */
    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_headers,
                   send_upstream_headers, ds_req);

    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_new_chunk,
                   send_upstream_new_chunk, ds_req);

    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_chunk_complete,
                   send_upstream_chunk_done, ds_req);

    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_chunks_complete,
                   send_upstream_chunks_done, ds_req);

    /* call this function if the upstream request contains a body */
    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_read,
                   send_upstream_body, ds_req);

    /* call this function after the upstream request has been marked as complete
     */
    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_request_fini,
                   upstream_fini, ds_req);

    /* call this function if the upstream request encounters a socket error */
    evhtp_set_hook(&up_req->hooks, evhtp_hook_on_error,
                   upstream_error, ds_req);

    evhtp_set_hook(&up_req->conn->hooks, evhtp_hook_on_write,
                   upstream_on_write, ds_req);

    /* insert this request into our pending queue and signal processor event */
    TAILQ_INSERT_TAIL(&rproxy->pending, ds_req, next);
    event_active(rproxy->request_ev, EV_WRITE, 1);

    /* tell evhtp to stop reading from the upstream socket */
    return EVHTP_RES_PAUSE;
} /* upstream_request_start */

/**
 * @brief the evthr init callback. Setup rproxy event base and initialize
 *         downstream connections.
 *
 * @param htp
 * @param thr
 * @param arg
 */
void
rproxy_thread_init(evhtp_t * htp, evthr_t * thr, void * arg) {
    evbase_t     * evbase;
    rproxy_t     * rproxy;
    server_cfg_t * server_cfg;
    int            res;

    assert(htp != NULL);
    assert(thr != NULL);

    server_cfg          = arg;
    assert(server_cfg != NULL);

    evbase              = evthr_get_base(thr);
    assert(evbase != NULL);

    rproxy              = calloc(sizeof(rproxy_t), 1);
    assert(rproxy != NULL);

    rproxy->req_log     = logger_init(server_cfg->req_log_cfg, 0);
    rproxy->err_log     = logger_init(server_cfg->err_log_cfg,
                                      LZLOG_OPT_WNAME | LZLOG_OPT_WPID | LZLOG_OPT_WDATE);

    rproxy->downstreams = lztq_new();
    assert(rproxy->downstreams != NULL);

    rproxy->rules       = lztq_new();
    assert(rproxy->rules != NULL);

    /* create a dns_base which is used for various resolution functions, e.g.,
     * bufferevent_socket_connect_hostname()
     */
    rproxy->dns_base    = evdns_base_new(evbase, 1);
    assert(rproxy->dns_base != NULL);

    /* init our pending request tailq */
    TAILQ_INIT(&rproxy->pending);

    rproxy->server_cfg = server_cfg;
    rproxy->evbase     = evbase;
    rproxy->htp        = htp;

    /* create a downstream_t instance for each configured downstream */
    res = lztq_for_each(server_cfg->downstreams, add_downstream, rproxy);
    assert(res == 0);

    /* enable the pending request processing event, this event is triggered
     * whenever a downstream connection becomes available.
     */
    rproxy->request_ev = event_new(evbase, -1, EV_READ | EV_PERSIST,
                                   rproxy_process_pending, rproxy);
    assert(rproxy->request_ev != NULL);

    /* set aux data argument to this threads specific rproxy_t structure */
    evthr_set_aux(thr, rproxy);

    /* start all of our downstream connections */
    res = lztq_for_each(rproxy->downstreams, start_downstream, evbase);
    assert(res == 0);

    {
        /* for each virtual server, iterate over each rule_cfg and create a
         * rule_t structure.
         *
         * Since each of these rule_t's are unique pointers, we append them
         * to the global rproxy->rules list (evhtp takes care of the vhost
         * matching, and the rule_cfg is passed as the argument to
         * upstream_request_start).
         *
         * Each rule_t has a downstreams list containing pointers to
         * (already allocated) downstream_t structures.
         */
        lztq_elem * vhost_cfg_elem = NULL;
        lztq_elem * vhost_cfg_temp = NULL;

        for (vhost_cfg_elem = lztq_first(server_cfg->vhosts); vhost_cfg_elem; vhost_cfg_elem = vhost_cfg_temp) {
            vhost_cfg_t * vhost_cfg = lztq_elem_data(vhost_cfg_elem);
            vhost_t     * vhost;

            vhost          = calloc(sizeof(vhost_t), 1);
            assert(vhost != NULL);

            /* initialize the vhost specific logging, these are used if a rule
             * does not have its own logging configuration. This allows for rule
             * specific logs, and falling back to a global one.
             */
            vhost->req_log = logger_init(vhost_cfg->req_log, 0);
            vhost->err_log = logger_init(vhost_cfg->err_log, 0);
            vhost->config  = vhost_cfg;
            vhost->rproxy  = rproxy;

            res = lztq_for_each(vhost_cfg->rule_cfgs, map_vhost_rules_to_downstreams, vhost);
            assert(res == 0);

            vhost_cfg_temp = lztq_next(vhost_cfg_elem);
        }
    }

    return;
} /* rproxy_thread_init */

/**
 * @brief Set an evhtp callback based on information in a single rule_cfg_t
 *        structure. Based on the rule type, we either use set_cb, set_regex_cb,
 *        or set_glob_cb. Only one real callback set is an on_hostname hook.
 *
 * @param elem
 * @param arg
 *
 * @return
 */
static int
add_callback_rule(lztq_elem * elem, void * arg) {
    evhtp_t          * htp  = arg;
    rule_cfg_t       * rule = lztq_elem_data(elem);
    evhtp_callback_t * cb   = NULL;

    switch (rule->type) {
        case rule_type_exact:
            cb = evhtp_set_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_regex:
            cb = evhtp_set_regex_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_glob:
            cb = evhtp_set_glob_cb(htp, rule->matchstr, NULL, rule);
            break;
        case rule_type_default:
            /* default rules will match anything */
            cb = evhtp_set_glob_cb(htp, "*", NULL, rule);
            break;
    }

    if (cb == NULL) {
        fprintf(stderr, "Could not compile evhtp callback for pattern \"%s\"\n", rule->matchstr);
        exit(EXIT_FAILURE);
    }

    /* if one of the callbacks matches, upstream_request_start will be called
     * with the argument of this rule_cfg_t
     */
    evhtp_set_hook(&cb->hooks, evhtp_hook_on_hostname,
                   upstream_request_start, rule);

    return 0;
}

static int
add_vhost_name(lztq_elem * elem, void * arg) {
    evhtp_t * htp_vhost = arg;
    char    * name      = lztq_elem_data(elem);

    assert(htp_vhost != NULL);
    assert(name != NULL);

    return evhtp_add_alias(htp_vhost, name);
}

static int
add_vhost(lztq_elem * elem, void * arg) {
    evhtp_t     * htp       = arg;
    vhost_cfg_t * vcfg      = lztq_elem_data(elem);
    evhtp_t     * htp_vhost = NULL;

    /* create a new child evhtp for this vhost */
    htp_vhost = evhtp_new(htp->evbase, NULL);

    /* disable 100-continue responses, we let the downstreams deal with this.
     */
    evhtp_disable_100_continue(htp_vhost);

    /* for each rule, create a evhtp callback with the defined type */
    lztq_for_each(vcfg->rule_cfgs, add_callback_rule, htp_vhost);

    /* add the vhost using the name of the vhost config directive */
    evhtp_add_vhost(htp, vcfg->server_name, htp_vhost);

    /* create an alias for the vhost for each configured alias directive */
    lztq_for_each(vcfg->aliases, add_vhost_name, htp_vhost);

    /* here we add a rule to rule them all. If no rules match, we want to be
     * able to shut the connection down so it doesn't consume any resources.
     *
     * If this is not done, evhtp will just keep consuming input data, and in
     * the case of a large POST request, memory will be eaten until complete.
     * This is bad.
     */
    {
        evhtp_callback_t * cb;

        if (!(cb = evhtp_set_glob_cb(htp_vhost, "*", NULL, htp_vhost))) {
            fprintf(stderr, "Could not alloc callback: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        evhtp_set_hook(&cb->hooks, evhtp_hook_on_hostname,
                       upstream_dump_request, htp_vhost);
    }

    if (vcfg->ssl_cfg != NULL) {
        /* vhost specific ssl configuration found */
        evhtp_ssl_init(htp_vhost, vcfg->ssl_cfg);

        /* if CRL checking is enabled, create a new ssl_crl_ent_t and add it
         * to the evhtp_t's arguments. XXX: in the future we should create a
         * generic wrapper for various things we want to put in the evhtp
         * arguments, but for now, the only thing that we care about is the
         * CRL context.
         */
        if (vcfg->ssl_cfg->args) {
            ssl_crl_cfg_t * crl_cfg = vcfg->ssl_cfg->args;

            htp->arg = (void *)ssl_crl_ent_new(htp, crl_cfg);
            assert(htp->arg != NULL);
        }
    } else if (htp->ssl_ctx != NULL) {
        /* use the global SSL context */
        htp_vhost->ssl_ctx = htp->ssl_ctx;
        htp_vhost->ssl_cfg = htp->ssl_cfg;
        htp_vhost->arg     = htp->arg;
    }

    return 0;
} /* add_vhost */

int
rproxy_init(evbase_t * evbase, rproxy_cfg_t * cfg) {
    lztq_elem * serv_elem;
    lztq_elem * serv_temp;

    assert(evbase != NULL);
    assert(cfg != NULL);

    /* iterate over each server_cfg, and set up evhtp stuff */
    for (serv_elem = lztq_first(cfg->servers); serv_elem != NULL; serv_elem = serv_temp) {
        struct timeval * tv_read  = NULL;
        struct timeval * tv_write = NULL;
        evhtp_t        * htp;
        server_cfg_t   * server;

        server = lztq_elem_data(serv_elem);
        assert(server != NULL);

        /* create a new evhtp base structure for just this server, all vhosts
         * will use this as a parent
         */
        htp    = evhtp_new(evbase, NULL);
        assert(htp != NULL);

        /* we want to make sure 100-continue is not sent by evhtp, but the
         * downstreams themselves.
         */
        evhtp_disable_100_continue(htp);

        /* create a pre-accept callback which will disconnect the client
         * immediately if the max-pending request queue is over the configured
         * limit.
         */
        evhtp_set_pre_accept_cb(htp, upstream_pre_accept, NULL);

        if (server->ssl_cfg) {
            /* enable SSL support on this server */
            evhtp_ssl_init(htp, server->ssl_cfg);

            /* if CRL checking is enabled, create a new ssl_crl_ent_t and add it
             * to the evhtp_t's arguments. XXX: in the future we should create a
             * generic wrapper for various things we want to put in the evhtp
             * arguments, but for now, the only thing that we care about is the
             * CRL context.
             */
            if (server->ssl_cfg->args) {
                ssl_crl_cfg_t * crl_cfg = server->ssl_cfg->args;

                htp->arg = ssl_crl_ent_new(htp, crl_cfg);
                assert(htp->arg != NULL);
            }
        }

        /* for each vhost, create a child virtual host and stick it in our main
         * evhtp structure.
         */
        lztq_for_each(server->vhosts, add_vhost, htp);

        /* if configured, set our upstream connection's read/write timeouts */
        if (server->read_timeout.tv_sec || server->read_timeout.tv_usec) {
            tv_read = &server->read_timeout;
        }

        if (server->write_timeout.tv_sec || server->write_timeout.tv_usec) {
            tv_write = &server->write_timeout;
        }

        if (tv_read || tv_write) {
            evhtp_set_timeouts(htp, tv_read, tv_write);
        }

        /* use a worker thread pool for connections, and for each
         * thread that is initialized call the rproxy_thread_init
         * function. rproxy_thread_init will create a new rproxy_t
         * instance for each of the threads
         */
        evhtp_use_threads(htp, rproxy_thread_init,
                          server->num_threads, server);

        if (evhtp_bind_socket(htp,
                              server->bind_addr,
                              server->bind_port,
                              server->listen_backlog) < 0) {
            fprintf(stderr, "[ERROR] unable to bind to %s:%d (%s)\n",
                    server->bind_addr,
                    server->bind_port,
                    strerror(errno));
            exit(-1);
        }

        if (server->disable_server_nagle == 1) {
            /* disable the tcp nagle algorithm for the listener socket */
            evutil_socket_t sock;

            sock = evconnlistener_get_fd(htp->server);
            assert(sock >= 0);

            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (int[]) { 1 }, sizeof(int));
        }


        serv_temp = lztq_next(serv_elem);
    }

    return 0;
}     /* rproxy_init */

static void
rproxy_process_pending(int fd, short which, void * arg) {
    rproxy_t  * rproxy;
    request_t * request;
    request_t * save;
    int         res;

    rproxy = arg;
    assert(rproxy != NULL);

    for (request = TAILQ_FIRST(&rproxy->pending); request; request = save) {
        evhtp_request_t    * us_req;
        evhtp_connection_t * us_conn;
        rule_cfg_t         * rule_cfg;

        save = TAILQ_NEXT(request, next);

        if (!(request->downstream_conn = downstream_connection_get(request->rule))) {
            continue;
        }

        /* set the connection to an active state so that other pending requests
         * do not get this same downstream connection.
         */
        res = downstream_connection_set_active(request->downstream_conn);
        assert(res >= 0);

        /* remove this request from the pending queue */
        TAILQ_REMOVE(&rproxy->pending, request, next);

        request->downstream_bev           = request->downstream_conn->connection;
        request->downstream_conn->request = request;
        request->pending   = 0;
        rproxy->n_pending -= 1;

        if (request->pending_ev != NULL) {
            /* delete the pending timer so that it does not trigger */
            evtimer_del(request->pending_ev);
        }

        /* since the upstream request processing has been paused, we must
         * unpause it to process it.
         */
        evhtp_request_resume(request->upstream_request);
    }
} /* rproxy_process_pending */

static void
rproxy_report_rusage(rproxy_rusage_t * rusage) {
    if (!rusage) {
        return;
    }

    printf("Configured resource usage information\n");
    printf("  Number of downstream connections : %u\n", rusage->total_num_connections);
    printf("  Number of threads                : %u\n", rusage->total_num_threads);
    printf("  Number of pending connections    : %u\n\n", rusage->total_max_pending);

    printf("Detailed resource rundown\n");
    printf("  A thread requires 2 fds            : %u\n", rusage->total_num_threads * 2);
    printf("  A ds connection is 1 fd * nthreads : %u\n",
           (rusage->total_num_connections * rusage->total_num_threads));
    printf("  Base fd resources needed           : %u\n\n",
           ((rusage->total_num_connections) * (rusage->total_num_threads * 2)));

    printf("  In a worst-case scenario where all pending queues are filled, you "
           "will need a bit more than %u descriptors available\n\n",
           ((rusage->total_num_connections) * (rusage->total_num_threads * 2)) +
           rusage->total_max_pending);

#ifndef NO_RLIMITS
    {
        struct rlimit limit;
        unsigned int  needed_fds;

        if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
            return;
        }

        needed_fds = ((rusage->total_num_connections) *
                      (rusage->total_num_threads * 2)) + rusage->total_max_pending;

        printf("  Your *CURRENT* rlimit is : %u\n", (unsigned int)limit.rlim_cur);
        printf("  Your *MAXIMUM* rlimit is : %u\n\n", (unsigned int)limit.rlim_max);

        if ((unsigned int)limit.rlim_cur < needed_fds) {
            printf("** WARNING: Needed resources exceed the current limits!\n");

            if ((unsigned int)limit.rlim_max > needed_fds) {
                printf("** WARNING: The good news is that you do have the "
                       "ability to obtain the resources! Try increasing the "
                       "option 'max-nofiles' in your configuration.\n");
            } else {
                printf("** WARNING: You must consult your operating systems "
                       "documentation in order to increase the resources "
                       "needed.\n");
            }
        }
    }
#endif
} /* rproxy_report_rusage */

int
main(int argc, char ** argv) {
    rproxy_cfg_t * rproxy_cfg;
    evbase_t     * evbase;

    if (argc < 2) {
        printf("RProxy Version: %s (Libevhtp Version: %s, Libevent Version: %s, OpenSSL Version: %s)\n",
               RPROXY_VERSION, EVHTP_VERSION, event_get_version(), SSLeay_version(SSLEAY_VERSION));
        fprintf(stderr, "Usage: %s <config>\n", argv[0]);
        return -1;
    }

    if (!(rproxy_cfg = rproxy_cfg_parse(argv[1]))) {
        fprintf(stderr, "Error parsing file %s\n", argv[1]);
        return -1;
    }

#ifdef USE_MALLOPT
    if (rproxy_cfg->mem_trimsz) {
        mallopt(M_TRIM_THRESHOLD, rproxy_cfg->mem_trimsz);
    }
#endif

    if (util_set_rlimits(rproxy_cfg->max_nofile) < 0) {
        exit(-1);
    }

    /* calculate and report on resources needed for the current configuration */
    rproxy_report_rusage(&rproxy_cfg->rusage);

    if (rproxy_cfg->daemonize == true) {
        if (util_daemonize(rproxy_cfg->rootdir, 1) < 0) {
            exit(-1);
        }
    }

    if (!(evbase = event_base_new())) {
        fprintf(stderr, "Error creating event_base: %s\n", strerror(errno));
        rproxy_cfg_free(rproxy_cfg);
        return -1;
    }

    rproxy_init(evbase, rproxy_cfg);

    if (rproxy_cfg->user || rproxy_cfg->group) {
        util_dropperms(rproxy_cfg->user, rproxy_cfg->group);
    }

    event_base_loop(evbase, 0);

    return 0;
} /* main */

