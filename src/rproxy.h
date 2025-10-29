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

#ifndef __RPROXY_H__
#define __RPROXY_H__

#include <stdbool.h>
#include <syslog.h>
#include <float.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <confuse.h>
#include <event2/dns.h>
#include <evhtp.h>
#include <netinet/tcp.h>
#include <pthread.h>

#include "lzq.h"
#include "lzlog.h"

#define RPROXY_VERSION "2.0.17"

#if EVHTP_VERSION_MAJOR <= 1
#if EVHTP_VERSION_MINOR <= 2
#if EVHTP_VERSION_PATCH < 3
#error RProxy requires libevhtp v1.2.3 or greater
#endif
#endif
#endif

/************************************************
* configuration structure definitions
************************************************/

enum rule_type {
    rule_type_exact,
    rule_type_regex,
    rule_type_glob,
    rule_type_default
};

enum lb_method {
    lb_method_rtt = 0,
    lb_method_rr,
    lb_method_rand,
    lb_method_most_idle,
    lb_method_none
};

enum logger_type {
    logger_type_file = 0,
    logger_type_syslog
};

typedef struct rproxy_rusage  rproxy_rusage_t;
typedef struct rproxy_cfg     rproxy_cfg_t;
typedef struct rule_cfg       rule_cfg_t;
typedef struct vhost_cfg      vhost_cfg_t;
typedef struct server_cfg     server_cfg_t;
typedef struct downstream_cfg downstream_cfg_t;
typedef struct headers_cfg    headers_cfg_t;
typedef struct x509_ext_cfg   x509_ext_cfg_t;
typedef struct ssl_crl_cfg    ssl_crl_cfg_t;
typedef struct logger_cfg     logger_cfg_t;

typedef enum rule_type        rule_type;
typedef enum lb_method        lb_method;
typedef enum logger_type      logger_type;

struct logger_cfg {
    lzlog_level level;
    logger_type type;
    char      * path;
    char      * format;
    int         facility;
};

struct rule_cfg {
    char          * name;            /**< the name of the rule */
    rule_type       type;            /**< what type of rule this is (regex/exact/glob) */
    lb_method       lb_method;       /**< method of load-balacinging (defaults to RTT) */
    char          * matchstr;        /**< the uri to match on */
    headers_cfg_t * headers;         /**< headers which are added to the backend request */
    lztq          * downstreams;     /**< list of downstream names (as supplied by downstream_cfg_t->name */
    logger_cfg_t  * req_log;         /**< request logging config */
    logger_cfg_t  * err_log;         /**< error logging config */
    bool            passthrough;     /**< if set to true, a pipe between the upstream and downstream is established */
    bool            allow_redirect;  /**< if true, the downstream can send a redirect to connect to a different downstream */
    lztq          * redirect_filter; /**< a list of hostnames that redirects are can connect to */
    int             has_up_read_timeout;
    int             has_up_write_timeout;
    struct timeval  up_read_timeout;
    struct timeval  up_write_timeout;
};

/**
 * @brief a configuration structure representing a single x509 extension header.
 *
 */
struct x509_ext_cfg {
    char * name;                 /**< the name of the header */
    char * oid;                  /**< the oid of the x509 extension to pull */
};

struct ssl_crl_cfg {
    char         * filename;
    char         * dirname;
    struct timeval reload_timer;
};

/**
 * @brief which headers to add to the downstream request if avail.
 */
struct headers_cfg {
    bool   x_forwarded_for;
    bool   x_ssl_subject;
    bool   x_ssl_issuer;
    bool   x_ssl_notbefore;
    bool   x_ssl_notafter;
    bool   x_ssl_sha1;
    bool   x_ssl_serial;
    bool   x_ssl_cipher;
    bool   x_ssl_certificate;
    lztq * x509_exts;
};

/**
 * @brief configuration for a single downstream.
 */
struct downstream_cfg {
    bool     enabled;               /**< true if server is enabled */
    char   * name;                  /**< the name of this downstream. the name is used as an identifier for rules */
    char   * host;                  /**< the hostname of the downstream */
    uint16_t port;                  /**< the port of the downstream */
    int      n_connections;         /**< number of connections to keep established */
    size_t   high_watermark;        /**< if the number of bytes pending on the output side
                                     * of the socket reaches this number, the proxy stops
                                     * reading from the upstream until all data has been written. */
    struct timeval retry_ival;      /**< retry timer if the downstream connection goes down */
    struct timeval read_timeout;
    struct timeval write_timeout;
};


struct vhost_cfg {
    evhtp_ssl_cfg_t * ssl_cfg;
    lztq            * rule_cfgs;        /**< list of rule_cfg_t's */
    lztq            * rules;            /* list of rule_t's */
    char            * server_name;
    lztq            * aliases;          /**< other hostnames this vhost is associated with */
    lztq            * strip_hdrs;       /**< headers to strip out from downstream responses */
    logger_cfg_t    * req_log;          /* request logging configuration */
    logger_cfg_t    * err_log;          /* error logging configuration */
    headers_cfg_t   * headers;          /**< headers which are added to the backend request */
};

/**
 * @brief configuration for a single listening frontend server.
 */
struct server_cfg {
    char   * bind_addr;                 /**< address to bind on */
    uint16_t bind_port;                 /**< port to bind on */
    int      num_threads;               /**< number of worker threads to start */
    int      max_pending;               /**< max pending requests before new connections are dropped */
    int      listen_backlog;            /**< listen backlog */
    size_t   high_watermark;            /**< upstream high-watermark */

    struct timeval read_timeout;        /**< time to wait for reading before client is dropped */
    struct timeval write_timeout;       /**< time to wait for writing before client is dropped */
    struct timeval pending_timeout;     /**< time to wait for a downstream to become available for a connection */

    rproxy_cfg_t    * rproxy_cfg;       /**< parent rproxy configuration */
    evhtp_ssl_cfg_t * ssl_cfg;          /**< if enabled, the ssl configuration */
    lztq            * downstreams;      /**< list of downstream_cfg_t's */
    lztq            * vhosts;           /**< list of vhost_cfg_t's */
    logger_cfg_t    * req_log_cfg;
    logger_cfg_t    * err_log_cfg;

    int disable_server_nagle;           /**< disable nagle for listening sockets */
    int disable_client_nagle;           /**< disable nagle for upstream sockets */
    int disable_downstream_nagle;       /**< disable nagle for downstream sockets */
};



/**
 * @brief This is a structure that is filled during the configuration parsing
 *        stage. The information contains the various resources (file descriptors
 *        and such) that would be needed for the proxy to run optimally.
 *
 *        The idea is to warn the administrator that his system limits
 *        may impact the performance of the service.
 */
struct rproxy_rusage {
    unsigned int total_num_connections; /**< the total of all downstream connections */
    unsigned int total_num_threads;     /**< the total threads which will spawn */
    unsigned int total_max_pending;     /**< the total of configured max-pending connections */
};

/**
 * @brief main configuration structure.
 */
struct rproxy_cfg {
    bool            daemonize;          /**< should proxy run in background */
    int             mem_trimsz;
    int             max_nofile;         /**< max number of open file descriptors */
    char          * rootdir;            /**< root dir to daemonize */
    char          * user;               /**< user to run as */
    char          * group;              /**< group to run as */
    lztq          * servers;            /**< list of server_cfg_t's */
    logger_cfg_t  * log;                /**< generic log configuration */
    rproxy_rusage_t rusage;             /**< the needed resource totals */
};

/********************************************
* Main structures
********************************************/

/**
 * @brief a downstream's connection status.
 */
enum downstream_status {
    downstream_status_nil = 0,          /**< connection has never been used */
    downstream_status_active,           /**< connection is actively processing */
    downstream_status_idle,             /**< connection is idle and available */
    downstream_status_down              /**< connection is down and cannot be used */
};

enum logger_argtype {
    logger_argtype_nil = 0,
    logger_argtype_src,
    logger_argtype_proxy,
    logger_argtype_ts,
    logger_argtype_ua,
    logger_argtype_meth,
    logger_argtype_uri,
    logger_argtype_proto,
    logger_argtype_status,
    logger_argtype_ref,
    logger_argtype_host,
    logger_argtype_ds_sport,
    logger_argtype_us_sport,
    logger_argtype_us_hdrval,
    logger_argtype_ds_hdrval,
    logger_argtype_rule,
    logger_argtype_printable
};

typedef struct rproxy            rproxy_t;
typedef struct downstream        downstream_t;
typedef struct downstream_c      downstream_c_t;
typedef struct request           request_t;
typedef struct rule              rule_t;
typedef struct vhost             vhost_t;
typedef struct logger_arg        logger_arg_t;
typedef struct logger            logger_t;
typedef struct pending_request_q pending_request_q_t;
typedef struct ssl_crl_ent       ssl_crl_ent_t;

typedef enum downstream_status   downstream_status;
typedef enum logger_argtype      logger_argtype;

#define REQUEST_HAS_ERROR(req) (req->error ? 1 : req->upstream_err ? 1 : 0)

struct logger_arg {
    logger_argtype type;
    char         * data;
    size_t         len;
    size_t         used;

    TAILQ_ENTRY(logger_arg) next;
};

struct logger {
    logger_cfg_t * config;
    lzlog        * log;

    TAILQ_HEAD(logger_args, logger_arg) args;
};

struct ssl_crl_ent {
    ssl_crl_cfg_t * cfg;
    rproxy_t      * rproxy;
    evhtp_t       * htp;
    X509_STORE    * crl;
    event_t       * reload_timer_ev;
#ifdef __APPLE__
    struct timespec last_file_mod;
    struct timespec last_dir_mod;
#else
    time_t last_file_mod;
    time_t last_dir_mod;
#endif
    pthread_mutex_t lock; /**< lock to make sure we don't overwrite during a verification */
};

struct vhost {
    vhost_cfg_t * config;
    rproxy_t    * rproxy;
    logger_t    * req_log;
    logger_t    * err_log;
};

/**
 * @brief structure which represents a full proxy request
 *
 */
struct request {
    rproxy_t        * rproxy;            /**< the parent rproxy_t structure */
    evhtp_request_t * upstream_request;  /**< the client request */
    evbev_t         * upstream_bev;
    downstream_c_t  * downstream_conn;   /**< the downstream connection */
    rule_t          * rule;              /**< the matched rule */
    htparser        * parser;            /**< htparser for responses from the downstream */
    event_t         * pending_ev;        /**< event timer for pending status */
    evbev_t         * downstream_bev;

    uint8_t error;                       /**< set of downstream returns some type of error */
    uint8_t upstream_err;                /**< set if the upstream encountered a socket error */
    uint8_t done;                        /**< request fully proxied and completed */
    uint8_t pending;                     /**< request is waiting for a downstream connection to be avail */
    uint8_t hit_highwm;
    uint8_t hit_upstream_highwm;
    uint8_t reading;

    TAILQ_ENTRY(request) next;
};

/**
 * @brief a structure representing a downstream connection.
 */
struct downstream_c {
    downstream_t    * parent;          /**< the parent downstream structure */
    evbev_t         * connection;      /**< the bufferevent connection */
    request_t       * request;         /**< the currently running request */
    event_t         * retry_timer;     /**< the timer event for reconnecting if down */
    downstream_status status;          /**< the status of this downstream */
    double            rtt;             /**< the last RTT for a request made to the connection */
    uint16_t          sport;           /**< the source port of the connected socket */
    uint8_t           bootstrapped;    /**< if not set to 1, the connection will immediately attempt the reconnect */
    struct timeval    tv_start;        /**< the time which the connection was set to active, used to calculate RTT */

    TAILQ_ENTRY(downstream_c) next;
};

/**
 * @brief a container active/idle/downed downstream connections
 */
struct downstream {
    downstream_cfg_t * config;         /**< this downstreams configuration */
    evbase_t         * evbase;
    rproxy_t         * rproxy;
    uint16_t           num_active;     /**< number of ents in the active list */
    uint16_t           num_idle;       /**< number of ents in the idle list */
    uint16_t           num_down;       /**< number of ents in the down list */

    TAILQ_HEAD(, downstream_c) active; /**< list of active connections */
    TAILQ_HEAD(, downstream_c) idle;   /**< list of idle and ready connections */
    TAILQ_HEAD(, downstream_c) down;   /**< list of connections which are down */
};

struct rule {
    rproxy_t   * rproxy;
    rule_cfg_t * config;
    vhost_t    * parent_vhost;         /**< the vhost this rule is under */
    lztq       * downstreams;          /**< list of downstream_t's configured for this rule */
    lztq_elem  * last_downstream_used; /**< the last downstream used to service a request. Used for round-robin loadbalancing */
    logger_t   * req_log;              /**< rule specific request log */
    logger_t   * err_log;              /**< rule specific error log */
};

TAILQ_HEAD(pending_request_q, request);

struct rproxy {
    rproxy_cfg_t      * config;
    evhtp_t           * htp;
    evbase_t          * evbase;
    struct evdns_base * dns_base;
    event_t           * request_ev;
    server_cfg_t      * server_cfg;
    logger_t          * request_log;              /* server specific request logging */
    logger_t          * error_log;                /* server specific error logging */
    lztq              * rules;
    lztq              * downstreams;              /**< list of all downstream_t's */
    int                 n_pending;                /**< number of pending requests */
    pending_request_q_t pending;                  /**< list of pending upstream request_t's */
    logger_t          * req_log;
    logger_t          * err_log;
};

/************************************************
* Configuration parsing function definitions.
************************************************/
rproxy_cfg_t * rproxy_cfg_parse(const char * filename);

/***********************************************
* Downstream handling functions
***********************************************/
downstream_t   * downstream_new(rproxy_t *, downstream_cfg_t *);
void             downstream_free(void *);

downstream_c_t * downstream_connection_new(evbase_t *, downstream_t *);
downstream_c_t * downstream_connection_get(rule_t *);
void             downstream_connection_free(downstream_c_t *);
int              downstream_connection_init(evbase_t *, downstream_t *);

int              downstream_connection_set_active(downstream_c_t *);
int              downstream_connection_set_idle(downstream_c_t *);
int              downstream_connection_set_down(downstream_c_t *);

downstream_t   * downstream_find_by_name(lztq *, const char *);

/* downstream socket handling callbacks */
void downstream_connection_eventcb(evbev_t *, short, void *);
void downstream_connection_retry(int, short, void *);

/********************************************
* SSL verification callback functions
********************************************/
int             ssl_x509_verifyfn(int, X509_STORE_CTX *);
int             ssl_x509_issuedcb(X509_STORE_CTX *, X509 *, X509 *);
ssl_crl_ent_t * ssl_crl_ent_new(evhtp_t *, ssl_crl_cfg_t *);



/***********************************************
* Request handling funcs (upstream/downstream)
***********************************************/
request_t * request_new(rproxy_t *);
void        request_free(request_t *);

/***********************************************
 * SSL helper functions.
 ************************************************/
unsigned char * ssl_subject_tostr(evhtp_ssl_t *);
unsigned char * ssl_issuer_tostr(evhtp_ssl_t *);
unsigned char * ssl_notbefore_tostr(evhtp_ssl_t *);
unsigned char * ssl_notafter_tostr(evhtp_ssl_t *);
unsigned char * ssl_sha1_tostr(evhtp_ssl_t *);
unsigned char * ssl_serial_tostr(evhtp_ssl_t *);
unsigned char * ssl_cipher_tostr(evhtp_ssl_t *);
unsigned char * ssl_cert_tostr(evhtp_ssl_t *);
unsigned char * ssl_x509_ext_tostr(evhtp_ssl_t *, const char *);

/***********************************************
 * Logging functions
 **********************************************/
logger_t * logger_init(logger_cfg_t * config, int opts);
void       logger_log(logger_t * logger, lzlog_level level, char * fmt, ...);
void       logger_log_request(logger_t * logger, request_t * request);
void       logger_log_request_error(logger_t * logger, request_t * request, char * fmt, ...);

/***********************************************
 * Utility functions.
 **********************************************/
void      util_dropperms(const char * user, const char * group);
int       util_daemonize(char * root, int noclose);
int       util_set_rlimits(int nofiles);

evbuf_t * util_request_to_evbuffer(evhtp_request_t * request);
int       util_write_header_to_evbuffer(evhtp_header_t * hdr, void * arg);

int       util_glob_match(const char * pattern, const char * string);
int       util_glob_match_lztq(lztq *, const char *);

int       util_rm_headers_via_lztq(lztq * tq, evhtp_headers_t * headers);

#endif

