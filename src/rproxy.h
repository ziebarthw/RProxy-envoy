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

#pragma once

#include <stdint.h>
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

#include <glib.h>
#include <gio/gio.h>

#include "lzlog.h"

G_BEGIN_DECLS

#define RPROXY_VERSION "2.0.17"

#if EVHTP_VERSION_MAJOR <= 1
#if EVHTP_VERSION_MINOR <= 2
#if EVHTP_VERSION_PATCH < 3
#error RProxy requires libevhtp v1.2.3 or greater
#endif
#endif
#endif

// Simple helper to check for a "zero" timeval.
#define IS_TIMEVAL_ZERO(tv) ((tv)->tv_sec == 0 && (tv)->tv_usec == 0)

// evbuffer_get_length() helper.
#define evbuf_length(b) ((b) ? evbuffer_get_length(b) : 0)

// Identifies "owned" pointers that are to be freed by said owner.
// Because this is standard C, this is for coder reference only - i.e. a guide
// to implementation semantics.
#ifndef UNIQUE_PTR
#define UNIQUE_PTR(p) p*
#endif

// Identifies a shared pointer that is intended to be destroyed when its
// reference count goes to zero.
// Because this is standard C, this is for coder reference only - i.e. a guide
// to implementation semantics.
#ifndef SHARED_PTR
#define SHARED_PTR(p) p*
#endif

// Identifies a weak pointer that is intended to indicate the holder does not
// know (or care) if the instance object pointed - typically declared as a
// SHARED_PTR - is detroyed when all references are gone (reference count
// goes to zero).
// Because this is standard C, this is for coder reference only - i.e. a guide
// to implementation semantics.
#ifndef WEAK_PTR
#define WEAK_PTR(p) p*
#endif

// Simple way to indicate a private function overrides a parent instance
// method.
#ifndef OVERRIDE
#define OVERRIDE static
#endif

#ifndef LOC_FMT
#define LOC_FMT "%s:%s"
#endif
#ifndef LOC_ARG
#define LOC_ARG G_STRLOC, G_STRFUNC
#endif

#ifndef NUL
#define NUL '\0'
#endif

/************************************************
* configuration structure definitions
************************************************/

typedef enum {
    RpDnsLookupFamily_AUTO,
    RpDnsLookupFamily_V4_ONLY,
    RpDnsLookupFamily_V6_ONLY,
    RpDnsLookupFamily_V4_PREFERRED,
    RpDnsLookupFamily_ALL
} RpDnsLookupFamily_e;

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

enum enc_type {
    enc_type_none = 0,
    enc_type_deflate,
    enc_type_gzip,
    enc_type_br
};

enum discovery_type {
    discovery_type_static = 0,
    discovery_type_strict_dns,
    discovery_type_local_dns,
    discovery_type_eds,
    discovery_type_original_dst
};

typedef struct evdns_base evdns_base_t;
typedef struct evdns_request evdns_request_t;

typedef struct _RpHttpConnectionManagerConfig RpHttpConnectionManagerConfig;
typedef struct _RpDispatcher RpDispatcher;
typedef UNIQUE_PTR(RpDispatcher) RpDispatcherPtr;
typedef struct _RpThreadLocalInstance RpThreadLocalInstance;
typedef struct _RpDownstreamTransportSocketFactory RpDownstreamTransportSocketFactory;
typedef UNIQUE_PTR(RpDownstreamTransportSocketFactory) RpDownstreamTransportSocketFactoryPtr;

typedef struct rproxy         rproxy_t;
typedef struct rproxy_rusage  rproxy_rusage_t;
typedef struct rproxy_cfg     rproxy_cfg_t;
typedef struct rule_cfg       rule_cfg_t;
typedef struct rule           rule_t;
typedef struct vhost_cfg      vhost_cfg_t;
typedef struct server_cfg     server_cfg_t;
typedef struct upstream_cfg   upstream_cfg_t;
typedef struct headers_cfg    headers_cfg_t;
typedef struct x509_ext_cfg   x509_ext_cfg_t;
typedef struct ssl_crl_cfg    ssl_crl_cfg_t;
typedef struct logger_cfg     logger_cfg_t;

typedef enum rule_type        rule_type;
typedef enum lb_method        lb_method;
typedef enum logger_type      logger_type;
typedef enum enc_type         enc_type;
typedef enum discovery_type   discovery_type;

struct logger_cfg {
    lzlog_level level;
    logger_type type;
    char      * path;
    char      * format;
    int         facility;
};

static inline void
dummy_freefn(gpointer arg G_GNUC_UNUSED)
{
}

typedef struct dfp_dns_cache_cfg dfp_dns_cache_cfg_t;
struct dfp_dns_cache_cfg {
    char * name;                           /**< the name of the cache */
    RpDnsLookupFamily_e dns_lookup_family; /**< the DNS lookup family to use during resolution */
    struct timeval host_ttl;               /**< the TTL for hosts that are unused */
    guint32 max_hosts;                     /**< the maximum number of hosts that the cache will hold */
    GSList * preresolve_hostnames;         /**< hostnames that should be preresolved into the cache upon creation */
};

typedef struct dfp_sub_clusters_cfg dfp_sub_clusters_cfg_t;
struct dfp_sub_clusters_cfg {
    lb_method lb_policy;            /**< the load balancer type to use when picking a host in a sub cluster */
    guint32 max_sub_clusters;       /**< the maximum number of sub clusters that the DFP cluster will hold */
    struct timeval sub_cluster_ttl; /**< the TTL for sub clusters that are unused */
    GSList * preresolve_clusters;   /**< sub clusters that should be created & warmed upon creation */
};

typedef struct dfp_cluster_cfg dfp_cluster_cfg_t;
struct dfp_cluster_cfg {
    dfp_dns_cache_cfg_t* dns_cache_cfg;
    dfp_sub_clusters_cfg_t* sub_clusters_cfg;
};

typedef struct cluster_type_cfg cluster_type_cfg_t;
struct cluster_type_cfg {
    char * name;
    dfp_cluster_cfg_t* dfp_cluster_config;
};

struct rule_cfg {
    char               * name;            /**< the name of the rule */
    rule_type            type;            /**< what type of rule this is (regex/exact/glob) */
    lb_method            lb_method;       /**< method of load-balacinging (defaults to RTT) */
    discovery_type       discovery_type;
    char               * matchstr;        /**< the uri to match on */
    headers_cfg_t      * headers;         /**< headers which are added to the backend request */
    GSList             * upstream_names;  /**< list of upstream names (as supplied by upstream_cfg_t->name */
    logger_cfg_t       * req_log;         /**< request logging config */
    logger_cfg_t       * err_log;         /**< error logging config */
    bool                 passthrough;     /**< if set to true, a pipe between the upstream and upstream is established */
    bool                 allow_redirect;  /**< if true, the upstream can send a redirect to connect to a different upstream */
    GSList             * redirect_filter; /**< a list of hostnames that redirects are can connect to */
    int                  has_up_read_timeout;
    int                  has_up_write_timeout;
    struct timeval       up_read_timeout;
    struct timeval       up_write_timeout;
    struct timeval       connect_timeout;
    cluster_type_cfg_t * cluster_type;    /**< custom cluster type */
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
 * @brief which headers to add to the upstream request if avail.
 */
struct headers_cfg {
    bool     x_forwarded_for;
    bool     x_ssl_subject;
    bool     x_ssl_issuer;
    bool     x_ssl_notbefore;
    bool     x_ssl_notafter;
    bool     x_ssl_sha1;
    bool     x_ssl_serial;
    bool     x_ssl_cipher;
    bool     x_ssl_certificate;
    GSList * x509_exts;
};

/**
 * @brief configuration for a single upstream.
 */
struct upstream_cfg {
    evhtp_ssl_cfg_t * ssl_cfg;      /**< if enabled, the ssl configuration */
    bool     enabled;               /**< true if server is enabled */
    char   * name;                  /**< the name of this upstream. the name is used as an identifier for rules */
    char   * host;                  /**< the hostname of the upstream */
    uint16_t port;                  /**< the port of the upstream */
    int      n_connections;         /**< number of connections to keep established */
    size_t   high_watermark;        /**< if the number of bytes pending on the output side
                                     * of the socket reaches this number, the proxy stops
                                     * reading from the upstream until all data has been written. */
    struct timeval retry_ival;      /**< retry timer if the upstream connection goes down */
    struct timeval read_timeout;
    struct timeval write_timeout;
};


struct vhost_cfg {
    evhtp_ssl_cfg_t  * ssl_cfg;
    GSList           * rule_cfgs;        /**< list of rule_cfg_t's */
    GSList           * rules;            /**< list of rule_t's */
    char             * server_name;
    GSList           * aliases;          /**< other hostnames this vhost is associated with */
    GSList           * strip_hdrs;       /**< headers to strip out from upstream responses */
    GSList           * rewrite_urls;     /**< urls to rewrite (incoming and outgoing?) */
    logger_cfg_t     * req_log;          /**< request logging configuration */
    logger_cfg_t     * err_log;          /**< error logging configuration */
    headers_cfg_t    * headers;          /**< headers which are added to the backend request */
    server_cfg_t     * server_cfg;       /**< parent server configuration */
};

/**
 * @brief configuration for a single listening frontend server.
 */
struct server_cfg {
    char   * name;
    char   * bind_addr;                 /**< address to bind on */
    uint16_t bind_port;                 /**< port to bind on */
    int      num_threads;               /**< number of worker threads to start */
    int      max_pending;               /**< max pending requests before new connections are dropped */
    int      listen_backlog;            /**< listen backlog */
    size_t   high_watermark;            /**< upstream high-watermark */

    gint worker_num;                    /**< simple zero-based index worker number */

    struct timeval read_timeout;        /**< time to wait for reading before client is dropped */
    struct timeval write_timeout;       /**< time to wait for writing before client is dropped */
    struct timeval pending_timeout;     /**< time to wait for a upstream to become available for a connection */

    rproxy_cfg_t    * rproxy_cfg;       /**< parent rproxy configuration */
    evhtp_ssl_cfg_t * ssl_cfg;          /**< if enabled, the ssl configuration */
    GSList          * upstream_cfgs;    /**< list of upstream_cfg_t's */
    GSList          * vhost_cfgs;       /**< list of vhost_cfg_t's */
    logger_cfg_t    * req_log_cfg;
    logger_cfg_t    * err_log_cfg;

    rule_cfg_t      * default_rule_cfg; /**< default rule_cfg_t */
    rule_t          * default_rule;     /**< default rule_t */

    evhtp_t* m_htp;
    evthr_pool_t* m_thr_pool;
    void* m_arg;

    bool disable_server_nagle : 1;      /**< disable nagle for listening sockets */
    bool disable_client_nagle : 1;      /**< disable nagle for upstream sockets */
    bool disable_upstream_nagle : 1;    /**< disable nagle for upstream sockets */
    bool enable_workers_listen : 1;     /**< enable worker thread listening */
};

static inline uint16_t
server_cfg_get_worker_num(server_cfg_t* self)
{
    uint16_t worker_num = g_atomic_int_add(&self->worker_num, 1);
    return worker_num % self->num_threads;
}


/**
 * @brief This is a structure that is filled during the configuration parsing
 *        stage. The information contains the various resources (file descriptors
 *        and such) that would be needed for the proxy to run optimally.
 *
 *        The idea is to warn the administrator that his system limits
 *        may impact the performance of the service.
 */
struct rproxy_rusage {
    unsigned int total_num_connections; /**< the total of all upstream connections */
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
    GSList        * servers;            /**< list of server_cfg_t's */
    const char    * filename;           /**< config file name */
    logger_cfg_t  * log;                /**< generic log configuration */
    rproxy_rusage_t rusage;             /**< the needed resource totals */
};

/********************************************
* Main structures
********************************************/

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

typedef struct tpool_ctx         tpool_ctx_t;
typedef struct upstream          upstream_t;
typedef struct upstream_c        upstream_c_t;
typedef struct vhost             vhost_t;
#ifdef WITH_LOGGER
typedef struct logger_arg        logger_arg_t;
typedef struct logger            logger_t;
#endif//WITH_LOGGER
typedef struct ssl_crl_ent       ssl_crl_ent_t;
typedef struct request_hooks     request_hooks_t;

typedef enum logger_argtype      logger_argtype;

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
#ifdef WITH_LOGGER
    logger_t    * req_log;
    logger_t    * err_log;
#endif//WITH_LOGGER
};

/**
 * @brief a structure representing a upstream connection.
 */
struct upstream_c {
    upstream_t    * parent;            /**< the parent upstream structure */
    evhtp_ssl_t     * ssl;
#ifdef WITH_RETRY_TIMER
    event_t         * retry_timer;     /**< the timer event for reconnecting if down */
#endif//WITH_RETRY_TIMER
    double            rtt;             /**< the last RTT for a request made to the connection */
    uint16_t          sport;           /**< the source port of the connected socket */
};

/**
 * @brief a container active/idle upstream connections
 */
struct upstream {
    upstream_cfg_t   * config;         /**< this upstreams configuration */
    evbase_t         * evbase;
    evhtp_ssl_ctx_t  * ssl_ctx;
    uint16_t           num_idle;       /**< number of ents in the idle list */

    double             m_rtt;          /**< the last RTT for a request made to the host */
};

struct rule {
    rule_cfg_t * config;
    vhost_t    * parent_vhost;         /**< the vhost this rule is under */
    GSList     * upstreams;            /**< list of upstream_t's configured for this rule */
    GSList     * last_upstream_used;   /**< the last upstream used to service a request. Used for round-robin loadbalancing */
#ifdef WITH_LOGGER
    logger_t   * req_log;              /**< rule specific request log */
    logger_t   * err_log;              /**< rule specific error log */
#endif//WITH_LOGGER
};

typedef enum
{
    RpWorkerContinue_CONN_MANAGER,
    RpWorkerContinue_DONE
} RpWorkerContinue_e;

struct rproxy {
    rproxy_t          * m_parent;
    tpool_ctx_t       * m_tpool_ctx;
    rproxy_cfg_t      * config;
    evhtp_t           * htp;
    server_cfg_t      * server_cfg;
#ifdef WITH_LOGGER
    logger_t          * request_log;              /* server specific request logging */
    logger_t          * error_log;                /* server specific error logging */
#endif//WITH_LOGGER
    GSList            * rules;
    GSList            * upstreams;                /**< list of all upstream_t's */
    GSList            * vhosts;                   /**< list of all vhost_t's */
    int                 n_pending;                /**< number of pending requests */
    int                 n_processing;             /**< number of in-flight requests */
    gint worker_num;
#ifdef WITH_LOGGER
    logger_t          * req_log;
    logger_t          * err_log;
#endif//WITH_LOGGER

    RpHttpConnectionManagerConfig* m_filter_config;
    RpDownstreamTransportSocketFactoryPtr m_transport_socket_factory;
    RpDispatcherPtr m_dispatcher;
    GList* m_active_connections;

    RpWorkerContinue_e m_state;
};
rproxy_t* rproxy_new(rproxy_t* parent, rproxy_cfg_t* config);
void rproxy_free(rproxy_t* self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(rproxy_t, rproxy_free)

/************************************************
* Configuration parsing function definitions.
************************************************/
rproxy_cfg_t * rproxy_cfg_parse(const char * filename);
bool rproxy_cfg_parse_server_buf(rproxy_cfg_t* rproxy_cfg,
                                    const char* server_cfg_buf);
void rproxy_cfg_free(rproxy_cfg_t * cfg);
upstream_cfg_t* upstream_cfg_parse(cfg_t* cfg);
server_cfg_t* server_cfg_parse(cfg_t* cfg);

/***********************************************
* upstream handling functions
***********************************************/
upstream_t     * upstream_new(upstream_cfg_t *);
void             upstream_free(void *);

upstream_c_t   * upstream_conn_new(evbase_t *, upstream_t *);
upstream_t     * upstream_get(rule_t *);
void             upstream_conn_free(upstream_c_t *);
int              upstream_conn_init(evbase_t *, upstream_t *);

upstream_t     * upstream_find_by_name(GSList *, const char *);

/********************************************
* SSL verification callback functions
********************************************/
int             ssl_x509_verifyfn(int, X509_STORE_CTX *);
int             ssl_x509_issuedcb(X509_STORE_CTX *, X509 *, X509 *);
ssl_crl_ent_t * ssl_crl_ent_new(evhtp_t *, ssl_crl_cfg_t *);

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
 * Utility functions.
 **********************************************/
#define UTIL_IS_DEFAULT_PORT(p) ((p) == 80 || (p) == 443)

void      util_dropperms(const char * user, const char * group);
int       util_daemonize(char * root, int noclose);
int       util_set_rlimits(int nofiles);

evbuf_t * util_request_to_evbuffer(evhtp_request_t * request, evbuf_t* obuf);
evbuf_t * util_response_to_evbuffer(evhtp_request_t* request, evbuf_t* obuf);
int       util_write_header_to_evbuffer(evhtp_header_t * hdr, void * arg);

bool      util_glob_match(const char * pattern, const char * string);
bool      util_glob_match_slist(GSList *, const char *);

int       util_rm_headers_via_slist(GSList * list, evhtp_headers_t * headers);

bool      util_uri_to_evbuffer(const evhtp_request_t* req,
                                const char* hostname,
                                evbuf_t* obuf);
bool      util_filename_is_uri(const char* filename);

const char* util_format_sockaddr_port(const struct sockaddr* sa,
                                        char* out,
                                        size_t outlen);

bool util_is_text_content_type(const char* value);
enc_type util_get_enc_type(const char* enc_value);
GInputStream* util_create_input_stream(GInputStream* base, enc_type type);

static inline const char*
util_htp_proto_to_modsec_version(evhtp_proto proto)
{
    switch (proto)
    {
        case EVHTP_PROTO_11:
            return "1.1";
        case EVHTP_PROTO_10:
            return "1.0";
        case EVHTP_PROTO_INVALID:
        default:
            return "invalid";
    }
}

typedef struct rproxy_hooks rproxy_hooks_t;
struct rproxy_hooks {
    bool (* on_cfg_init)(rproxy_t*, void*);
    void (* on_thread_init)(rproxy_t*, void*);
    void (* on_thread_exit)(rproxy_t*, void*);
    void (* on_sigint)(rproxy_t*, void*);

    void* on_cfg_init_arg;
    void* on_thread_init_arg;
    void* on_thread_exit_arg;
    void* on_sigint_arg;
};
static inline rproxy_hooks_t
rproxy_hooks_ctor(bool (* cfg_init)(rproxy_t*, void*), void* cfg_init_arg,
                    void (* thread_init)(rproxy_t*, void*), void* thread_init_arg,
                    void (* thread_exit)(rproxy_t*, void*), void* thread_exit_arg,
                    void (* sigint)(rproxy_t*, void*), void* sigint_arg)
{
    rproxy_hooks_t hooks = {
        .on_cfg_init = cfg_init,
        .on_thread_init = thread_init,
        .on_thread_exit = thread_exit,
        .on_sigint = sigint,
        .on_cfg_init_arg = cfg_init_arg,
        .on_thread_init_arg = thread_init_arg,
        .on_thread_exit_arg = thread_exit_arg,
        .on_sigint_arg = sigint_arg
    };
    return hooks;
}
int rproxy_main(int argc, char** argv, const rproxy_hooks_t* hooks);
bool rproxy_add_server_cfg(rproxy_t* rproxy, const char* server_cfg_buf);



typedef struct _RpCond RpCond;
struct _RpCond {
    GMutex m_mutex;
    GCond m_cond;
    guint m_signaled;
};
static inline RpCond*
rp_cond_init(RpCond* self)
{
    g_return_val_if_fail(self != NULL, NULL);
    g_mutex_init(&self->m_mutex);
    g_cond_init(&self->m_cond);
    self->m_signaled = 0;
    return self;
}
static inline void
rp_cond_clear(RpCond* self)
{
    g_return_if_fail(self != NULL);
    g_mutex_clear(&self->m_mutex);
    g_cond_clear(&self->m_cond);
}
static inline void
rp_cond_signal(RpCond* self)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->m_mutex);
    ++self->m_signaled;
    g_cond_signal(&self->m_cond);
    g_mutex_unlock(&self->m_mutex);
}
static inline void
rp_cond_wait(RpCond* self)
{
    g_return_if_fail(self != NULL);
    g_mutex_lock(&self->m_mutex);
    while (!self->m_signaled)
    {
        g_cond_wait(&self->m_cond, &self->m_mutex);
    }
    --self->m_signaled;
    g_mutex_unlock(&self->m_mutex);
}

#define RP_DEFINE_PAIR_STRICT(Name, T1, T2) \
  typedef struct {                          \
    T1 first;                               \
    T2 second;                              \
  } Name
#define RP_DECLARE_PAIR_CTOR(Name, T1, T2) \
  static inline Name                       \
  Name##_make(T1 a, T2 b)                  \
  {                                        \
    return (Name){ a, b };                 \
  }


/**
 * Interface for host application to extend configuration properties in key
 * libconfuse sections (currently server, server:vhost, server:vhost:rule).
 */
#if 0
typedef void (*rproxy_rule_ext_parse_fn)(rule_cfg_t *dst, cfg_t *rule_cfg, void *u);

typedef struct rproxy_cfg_ext_s rproxy_cfg_ext_t;
struct rproxy_cfg_ext_s {
  const cfg_opt_t *server_extra; /* appended inside server { } */
  const cfg_opt_t *vhost_extra;  /* appended inside vhost <title> { } */
  const cfg_opt_t *rule_extra;   /* appended inside rule <title> { } (applies to BOTH server:rule and vhost:rule) */

  rproxy_rule_ext_parse_fn rule_cb; /* optional: consume rule_extra into rule_cfg_t */
  gpointer arg;
};

rproxy_cfg_t* rproxy_cfg_parse_ex(const char* filename, const rproxy_cfg_ext_t* ext);
#endif//0

// ::google::protobuf::Duration internal representation.
typedef struct duration duration_t;
struct duration {
    gint64 seconds;
    gint32 nanos;
};


static inline void
rp_gobject_set_take(GObject** dst, GObject** srcptr)
{
    g_return_if_fail(dst != NULL);
    g_return_if_fail(srcptr != NULL);
    if (*dst) g_object_unref(*dst);
    *dst = g_steal_pointer(srcptr); // Takes ownership, src becomes NULL.
}
#define RP_TAKE_OBJ(dst, srcptr) \
    rp_gobject_set_take((GObject**)dst, (GObject**)srcptr)

static inline void
rp_gobject_set_share(GObject** dst, GObject* src_obj)
{
    g_return_if_fail(dst != NULL);
    g_set_object(dst, src_obj);
}
#define RP_SHARE_OBJ(dst, src) \
    rp_gobject_set_share((GObject**)dst, (GObject*)src)


G_END_DECLS
