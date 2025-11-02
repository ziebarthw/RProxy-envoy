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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rproxy.h"

#define DEFAULT_CIPHERS "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-RC4-SHA:ECDHE-RSA-AES128-SHA:RC4-SHA:RC4-MD5:ECDHE-RSA-AES256-SHA:AES256-SHA:ECDHE-RSA-DES-CBC3-SHA:DES-CBC3-SHA:AES128-SHA"

/* used to keep track of to-be-needed rlimit information, to be used later to
 * determine if the system settings can handle what is configured.
 */
static rproxy_rusage_t _rusage = { 0, 0, 0 };

static cfg_opt_t       ssl_crl_opts[] = {
    CFG_STR("file",        NULL,         CFGF_NONE),
    CFG_STR("dir",         NULL,         CFGF_NONE),
    CFG_INT_LIST("reload", "{ 10, 0  }", CFGF_NONE),
    CFG_END()
};

static cfg_opt_t       ssl_opts[] = {
    CFG_BOOL("enabled",           cfg_false,       CFGF_NONE),
    CFG_STR_LIST("protocols-on",  "{ALL}",         CFGF_NONE),
    CFG_STR_LIST("protocols-off", NULL,            CFGF_NONE),
    CFG_STR("cert",               NULL,            CFGF_NONE),
    CFG_STR("key",                NULL,            CFGF_NONE),
    CFG_STR("ca",                 NULL,            CFGF_NONE),
    CFG_STR("capath",             NULL,            CFGF_NONE),
    CFG_STR("ciphers",            DEFAULT_CIPHERS, CFGF_NONE),
    CFG_STR("sni",                NULL,            CFGF_NONE),
    CFG_BOOL("verify-peer",       cfg_false,       CFGF_NONE),
    CFG_BOOL("enforce-peer-cert", cfg_false,       CFGF_NONE),
    CFG_INT("verify-depth",       0,               CFGF_NONE),
    CFG_INT("context-timeout",    172800,          CFGF_NONE),
    CFG_BOOL("cache-enabled",     cfg_true,        CFGF_NONE),
    CFG_INT("cache-timeout",      1024,            CFGF_NONE),
    CFG_INT("cache-size",         65535,           CFGF_NONE),
    CFG_SEC("crl",                ssl_crl_opts,    CFGF_NODEFAULT|CFGF_IGNORE_UNKNOWN),
    CFG_END()
};

static cfg_opt_t       log_opts[] = {
    CFG_BOOL("enabled", cfg_false,                   CFGF_NONE),
    CFG_STR("output",   "file:/dev/stdout",          CFGF_NONE),
    CFG_STR("level",    "error",                     CFGF_NONE),
    CFG_STR("format",   "{SRC} {HOST} {URI} {HOST}", CFGF_NONE),
    CFG_END()
};

static cfg_opt_t       logging_opts[] = {
    CFG_SEC("request", log_opts, CFGF_NONE),
    CFG_SEC("error",   log_opts, CFGF_NONE),
    CFG_SEC("general", log_opts, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t       upstream_opts[] = {
    CFG_BOOL("enabled",           cfg_true,       CFGF_NONE),
    CFG_SEC("ssl",                ssl_opts,       CFGF_NODEFAULT),
    CFG_STR("addr",               NULL,           CFGF_NODEFAULT),
    CFG_INT("port",               0,              CFGF_NODEFAULT),
    CFG_INT("connections",        1,              CFGF_NONE),
    CFG_INT("high-watermark",     0,              CFGF_NONE),
    CFG_INT_LIST("read-timeout",  "{ 0, 0 }",     CFGF_NONE),
    CFG_INT_LIST("write-timeout", "{ 0, 0 }",     CFGF_NONE),
    CFG_INT_LIST("retry",         "{ 0, 50000 }", CFGF_NONE),
    CFG_END()
};

static cfg_opt_t       x509_ext_opts[] = {
    CFG_STR("name", NULL, CFGF_NONE),
    CFG_STR("oid",  NULL, CFGF_NONE),
    CFG_END()
};

static cfg_opt_t       headers_opts[] = {
    CFG_BOOL("x-forwarded-for",   cfg_true,      CFGF_NONE),
    CFG_BOOL("x-ssl-subject",     cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-issuer",      cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-notbefore",   cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-notafter",    cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-serial",      cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-sha1",        cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-cipher",      cfg_false,     CFGF_NONE),
    CFG_BOOL("x-ssl-certificate", cfg_true,      CFGF_NONE),
    CFG_SEC("x509-extension",     x509_ext_opts, CFGF_MULTI),
    CFG_END()
};

static cfg_opt_t       rule_opts[] = {
    CFG_STR("uri-match",                   NULL,         CFGF_NODEFAULT),
    CFG_STR("uri-gmatch",                  NULL,         CFGF_NODEFAULT),
    CFG_STR("uri-rmatch",                  NULL,         CFGF_NODEFAULT),
    CFG_STR_LIST("upstreams",              NULL,         CFGF_NODEFAULT),
    CFG_STR("lb-method",                   "rtt",        CFGF_NONE),
    CFG_STR("discovery-type",              "static",     CFGF_NONE),
    CFG_SEC("headers",                     headers_opts, CFGF_NODEFAULT),
    CFG_INT_LIST("upstream-read-timeout",  NULL,         CFGF_NODEFAULT),
    CFG_INT_LIST("upstream-write-timeout", NULL,         CFGF_NODEFAULT),
    CFG_BOOL("passthrough",                cfg_false,    CFGF_NONE),
    CFG_BOOL("allow-redirect",             cfg_false,    CFGF_NONE),
    CFG_STR_LIST("redirect-filter",        NULL,         CFGF_NODEFAULT),
    CFG_END()
};

static cfg_opt_t       vhost_opts[] = {
    CFG_SEC("ssl",                   ssl_opts,     CFGF_NODEFAULT),
    CFG_STR_LIST("aliases",          NULL,         CFGF_NONE),
    CFG_STR_LIST("strip-headers",    "{}",         CFGF_NONE),
    CFG_STR_LIST("rewrite-urls",     "{}",         CFGF_NONE),
    CFG_SEC("logging",               logging_opts, CFGF_NODEFAULT|CFGF_IGNORE_UNKNOWN),
    CFG_SEC("headers",               headers_opts, CFGF_NODEFAULT),
    CFG_SEC("rule",                  rule_opts,    CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_END()
};

static cfg_opt_t       server_opts[] = {
    CFG_STR("addr",                      "127.0.0.1",     CFGF_NONE),
    CFG_INT("port",                      8080,            CFGF_NONE),
    CFG_INT("threads",                   -1,              CFGF_NONE),
    CFG_INT_LIST("read-timeout",         "{ 0, 0 }",      CFGF_NONE),
    CFG_INT_LIST("write-timeout",        "{ 0, 0 }",      CFGF_NONE),
    CFG_INT_LIST("pending-timeout",      "{ 0, 0 }",      CFGF_NONE),
    CFG_INT("high-watermark",            0,               CFGF_NONE),
    CFG_INT("max-pending",               0,               CFGF_NONE),
    CFG_INT("backlog",                   1024,            CFGF_NONE),
    CFG_SEC("upstream",                  upstream_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NO_TITLE_DUPES),
    CFG_SEC("vhost",                     vhost_opts,      CFGF_MULTI | CFGF_TITLE | CFGF_NO_TITLE_DUPES),
    CFG_SEC("ssl",                       ssl_opts,        CFGF_NODEFAULT|CFGF_IGNORE_UNKNOWN),
    CFG_SEC("logging",                   logging_opts,    CFGF_NODEFAULT|CFGF_IGNORE_UNKNOWN),
    CFG_BOOL("disable-server-nagle",     cfg_false,       CFGF_NONE),
    CFG_BOOL("disable-client-nagle",     cfg_false,       CFGF_NONE),
    CFG_BOOL("disable-upstream-nagle",   cfg_false,       CFGF_NONE),
    CFG_BOOL("enable-workers-listen",    cfg_false,       CFGF_NONE),
    CFG_SEC("rule",                      rule_opts,       CFGF_TITLE | CFGF_MULTI | CFGF_NO_TITLE_DUPES),
    CFG_END()
};

static cfg_opt_t       rproxy_opts[] = {
    CFG_BOOL("daemonize", cfg_false,    CFGF_NONE),
    CFG_STR("rootdir",    "/tmp",       CFGF_NONE),
    CFG_STR("user",       NULL,         CFGF_NONE),
    CFG_STR("group",      NULL,         CFGF_NONE),
    CFG_INT("max-nofile", 1024,         CFGF_NONE),
    CFG_SEC("logging",    logging_opts, CFGF_NODEFAULT|CFGF_IGNORE_UNKNOWN),
    CFG_SEC("server",     server_opts,  CFGF_MULTI | CFGF_TITLE | CFGF_NO_TITLE_DUPES),
    CFG_END()
};

#ifdef WITH_JSON

{
    "daemonize": false,
    "rootdir": "/tmp",
    "user": null,
    "group": null,
    "max-nofile", 1024,
    "logging": {
        "request": {
            "enabled": false,
            "output": "file:/dev/stdout",
            "level": "error",
            "format": "{SRC} {HOST} {URI} {HOST}"
        },
        "error": {
            "enabled": false,
            "output": "file:/dev/stderr",
            "level": "error",
            "format": "{SRC} {HOST} {URI} {HOST}"
        },
        "general": {
            "enabled": false,
            "output": "file:/dev/stdout",
            "level": "error",
            "format": "{SRC} {HOST} {URI} {HOST}"
        },
    },
    "server": {

    }
}

#endif//WITH_JSON

struct {
    int          facility;
    const char * str;
} facility_strmap[] = {
    { LOG_KERN,     "kern"     },
    { LOG_USER,     "user"     },
    { LOG_MAIL,     "mail"     },
    { LOG_DAEMON,   "daemon"   },
    { LOG_AUTH,     "auth"     },
    { LOG_SYSLOG,   "syslog"   },
    { LOG_LPR,      "lptr"     },
    { LOG_NEWS,     "news"     },
    { LOG_UUCP,     "uucp"     },
    { LOG_CRON,     "cron"     },
    { LOG_AUTHPRIV, "authpriv" },
    { LOG_FTP,      "ftp"      },
    { LOG_LOCAL0,   "local0"   },
    { LOG_LOCAL1,   "local1"   },
    { LOG_LOCAL2,   "local2"   },
    { LOG_LOCAL3,   "local3"   },
    { LOG_LOCAL4,   "local4"   },
    { LOG_LOCAL5,   "local5"   },
    { LOG_LOCAL6,   "local6"   },
    { LOG_LOCAL7,   "local7"   },
    { -1,           NULL       }
};

static void ssl_cfg_free(evhtp_ssl_cfg_t * c);

/**
 * @brief Convert the config value of "lb-method" to a lb_method enum type.
 *
 * @param lbstr
 *
 * @return the lb_method enum
 */
static lb_method
lbstr_to_lbtype(const char* lbstr)
{
    LOGD("(%p(%s))", lbstr, lbstr);

    if (!lbstr)
    {
        LOGD("lbstr is null");
        return lb_method_rtt;
    }

    if (!strcasecmp(lbstr, "rtt"))
    {
        LOGD("rtt");
        return lb_method_rtt;
    }

    if (!strcasecmp(lbstr, "roundrobin"))
    {
        LOGD("roundrobin");
        return lb_method_rr;
    }

    if (!strcasecmp(lbstr, "random"))
    {
        LOGD("random");
        return lb_method_rand;
    }

    if (!strcasecmp(lbstr, "most-idle"))
    {
        LOGD("most-idle");
        return lb_method_most_idle;
    }

    if (!strcasecmp(lbstr, "none"))
    {
        LOGD("none");
        return lb_method_none;
    }

    LOGD("defaulting to rtt");
    return lb_method_rtt;
}

static discovery_type
discovery_type_str_to_discovery_type(const char* str)
{
    LOGD("(%p(%s))", str, str);

    if (!str)
    {
        LOGD("str is null");
        return discovery_type_static;
    }

    if (g_ascii_strcasecmp(str, "static") == 0)
    {
        LOGD("static");
        return discovery_type_static;
    }

    if (g_ascii_strcasecmp(str, "strict_dns") == 0)
    {
        LOGD("strict_dns");
        return discovery_type_strict_dns;
    }

    if (g_ascii_strcasecmp(str, "local_dns") == 0)
    {
        LOGD("local_dns");
        return discovery_type_local_dns;
    }

    if (g_ascii_strcasecmp(str, "eds") == 0)
    {
        LOGD("eds");
        return discovery_type_eds;
    }

    if (g_ascii_strcasecmp(str, "original_dst") == 0)
    {
        LOGD("original_dst");
        return discovery_type_original_dst;
    }

    LOGD("defaulting to static");
    return discovery_type_static;
}

logger_cfg_t*
logger_cfg_new(void)
{
    LOGD("()");
    return (logger_cfg_t *)g_new0(logger_cfg_t, 1);
}

void
logger_cfg_free(logger_cfg_t * lcfg)
{
    LOGD("(%p)", lcfg);

    if (!lcfg)
    {
        LOGD("lcfg is null");
        return;
    }

    g_clear_pointer(&lcfg->path, g_free);
    g_clear_pointer(&lcfg->format, g_free);

    g_free(lcfg);
}

rproxy_cfg_t*
rproxy_cfg_new(const char* filename)
{
    LOGD("(%p(%s))", filename, filename);

    rproxy_cfg_t* cfg = g_new0(rproxy_cfg_t, 1);
    g_assert(cfg != NULL);

    cfg->filename = filename;

    cfg->servers = lztq_new();
    g_assert(cfg->servers != NULL);

    LOGD("cfg %p", cfg);
    return cfg;
}

void
rproxy_cfg_free(rproxy_cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->rootdir, g_free);
    g_clear_pointer(&cfg->user, g_free);
    g_clear_pointer(&cfg->group, g_free);
    g_clear_pointer(&cfg->servers, lztq_free);
    g_free(cfg);
}

headers_cfg_t*
headers_cfg_new(void)
{
    LOGD("()");

    headers_cfg_t* c = g_new0(headers_cfg_t, 1);
    g_assert(c != NULL);

    c->x_forwarded_for   = false;
    c->x_ssl_subject     = false;
    c->x_ssl_issuer      = false;
    c->x_ssl_notbefore   = false;
    c->x_ssl_notafter    = false;
    c->x_ssl_cipher      = false;
    c->x_ssl_certificate = false;

    c->x509_exts         = lztq_new();
    g_assert(c->x509_exts != NULL);

    LOGD("c %p", c);
    return c;
}

void
headers_cfg_free(headers_cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->x509_exts, lztq_free);
    g_free(cfg);
}

vhost_cfg_t*
vhost_cfg_new(void)
{
    LOGD("()");

    vhost_cfg_t* cfg = g_new0(vhost_cfg_t, 1);
    g_assert(cfg != NULL);

    cfg->rule_cfgs = lztq_new();
    g_assert(cfg->rule_cfgs != NULL);

    cfg->rules     = lztq_new();
    g_assert(cfg->rules != NULL);

    cfg->aliases   = lztq_new();
    g_assert(cfg->aliases != NULL);

    LOGD("cfg %p", cfg);
    return cfg;
}

void
vhost_cfg_free(void* arg)
{
    LOGD("(%p)", arg);

    vhost_cfg_t * cfg = arg;
    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->ssl_cfg, ssl_cfg_free);
    g_clear_pointer(&cfg->rules, lztq_free);
    g_clear_pointer(&cfg->rule_cfgs, lztq_free);
    g_clear_pointer(&cfg->strip_hdrs, lztq_free);
    g_clear_pointer(&cfg->rewrite_urls, lztq_free);
    g_free(cfg);
}

server_cfg_t*
server_cfg_new(void)
{
    LOGD("()");

    server_cfg_t* cfg = g_new0(server_cfg_t, 1);
    g_assert(cfg != NULL);

    cfg->upstreams = lztq_new();
    g_assert(cfg->upstreams != NULL);

    cfg->vhosts = lztq_new();
    g_assert(cfg->vhosts != NULL);

    cfg->worker_num = 0;

    /* cfg->rules = lztq_new(); */

    LOGD("cfg %p", cfg);
    return cfg;
}

void
server_cfg_free(void* arg)
{
    LOGD("(%p)", arg);

    server_cfg_t* cfg = arg;
    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->name, g_free);
    g_clear_pointer(&cfg->bind_addr, g_free);
    g_clear_pointer(&cfg->ssl_cfg, ssl_cfg_free);
    g_clear_pointer(&cfg->vhosts, lztq_free);
    g_clear_pointer(&cfg->upstreams, lztq_free);
    g_free(cfg);
}

rule_cfg_t*
rule_cfg_new(void)
{
    LOGD("()");

    rule_cfg_t* cfg = g_new0(rule_cfg_t, 1);
    g_assert(cfg != NULL);

    cfg->upstreams = lztq_new();
    g_assert(cfg != NULL);

    LOGD("cfg %p", cfg);
    return cfg;
}

void
rule_cfg_free(void* arg)
{
    LOGD("(%p)", arg);

    rule_cfg_t* cfg = arg;
    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->headers, headers_cfg_free);
    g_clear_pointer(&cfg->upstreams, lztq_free);
    g_clear_pointer(&cfg->matchstr, g_free);
    g_clear_pointer(&cfg->redirect_filter, lztq_free);
    g_free(cfg);
}

upstream_cfg_t*
upstream_cfg_new(void)
{
    LOGD("()");
    return g_new0(upstream_cfg_t, 1);
}

void
upstream_cfg_free(void* arg)
{
    LOGD("(%p)", arg);

    upstream_cfg_t * cfg = arg;
    if (!cfg)
    {
        LOGD("cfg is null");
        return;
    }

    g_clear_pointer(&cfg->ssl_cfg, ssl_cfg_free);
    g_clear_pointer(&cfg->name, g_free);
    g_clear_pointer(&cfg->host, g_free);

    g_free(cfg);
}

/**
 * @brief allocate a new ssl configuration resource
 *
 * @return evhtp_ssl_cfg_t * on success, NULL on error.
 */
evhtp_ssl_cfg_t*
ssl_cfg_new(void)
{
    LOGD("()");
    return g_new0(evhtp_ssl_cfg_t, 1);
}

/**
 * @brief free ssl configuration resources
 *
 * @param c
 */
void
ssl_cfg_free(evhtp_ssl_cfg_t * c)
{
    LOGD("(%p)", c);

    if (c == NULL)
    {
        LOGD("c is null");
        return;
    }

    g_clear_pointer(&c->pemfile, g_free);
    g_clear_pointer(&c->privfile, g_free);
    g_clear_pointer(&c->cafile, g_free);
    g_clear_pointer(&c->capath, g_free);
    g_clear_pointer(&c->ciphers, g_free);

    g_free(c);
}

x509_ext_cfg_t*
x509_ext_cfg_new(void)
{
    LOGD("()");
    return g_new0(x509_ext_cfg_t, 1);
}

void
x509_ext_cfg_free(void* arg)
{
    LOGD("(%p)", arg);

    x509_ext_cfg_t* c = arg;
    if (c == NULL)
    {
        LOGD("c is null");
        return;
    }

    g_clear_pointer(&c->name, g_free);
    g_clear_pointer(&c->oid, g_free);

    g_free(c);
}

logger_cfg_t*
logger_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    logger_cfg_t* lcfg = logger_cfg_new();
    if (!lcfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    /* convert the level value from a string to the correct lzlog level enum */
    lcfg->level = lzlog_emerg;

    char* level_str = cfg_getstr(cfg, "level");
    if (level_str != NULL)
    {
        if (!strcasecmp(level_str, "emerg"))
        {
            LOGD("emerg");
            lcfg->level = lzlog_emerg;
        }
        else if (!strcasecmp(level_str, "alert"))
        {
            LOGD("alert");
            lcfg->level = lzlog_alert;
        }
        else if (!strcasecmp(level_str, "crit"))
        {
            LOGD("crit");
            lcfg->level = lzlog_crit;
        }
        else if (!strcasecmp(level_str, "error"))
        {
            LOGD("error");
            lcfg->level = lzlog_err;
        }
        else if (!strcasecmp(level_str, "warn"))
        {
            LOGD("warn");
            lcfg->level = lzlog_warn;
        }
        else if (!strcasecmp(level_str, "notice"))
        {
            LOGD("notice");
            lcfg->level = lzlog_notice;
        }
        else if (!strcasecmp(level_str, "info"))
        {
            LOGD("info");
            lcfg->level = lzlog_info;
        }
        else if (!strcasecmp(level_str, "debug"))
        {
            LOGD("debug");
            lcfg->level = lzlog_debug;
        }
        else
        {
            LOGD("defaulting to emerg");
            lcfg->level = lzlog_emerg;
        }
    }

    /* the output configuration directive is in the format of 'scheme:path'. If
     * the scheme is 'file', the path is the filename. If the scheme is
     * 'syslog', the path is the syslog facility.
     */
    g_autofree char* output_str = g_strdup(cfg_getstr(cfg, "output"));
    g_assert(output_str != NULL);

    char* scheme = strtok(output_str, ":");
    if (!strcasecmp(scheme, "file"))
    {
        lcfg->type = logger_type_file;
    }
    else if (!strcasecmp(scheme, "syslog"))
    {
        lcfg->type = logger_type_syslog;
    }
    else
    {
        lcfg->type = logger_type_file;
    }

    char* path = strtok(NULL, ":");
    switch (lcfg->type)
    {
        case logger_type_file:
            lcfg->path = g_strdup(path);
            break;
        case logger_type_syslog:
            for (int i = 0; facility_strmap[i].str; i++)
            {
                if (!strcasecmp(facility_strmap[i].str, path))
                {
                    lcfg->facility = facility_strmap[i].facility;
                }
            }
            break;
        default:
            break;
    }

    lcfg->format = strdup(cfg_getstr(cfg, "format"));
    if (!lcfg->format)
    {
        LOGE("alloc failed");
        logger_cfg_free(lcfg);
        return NULL;
    }

    return lcfg;
} /* logger_cfg_parse */

static inline bool
section_exists(cfg_t* cfg, const char* name, cfg_t** scfg)
{
    *scfg = cfg_getsec(cfg, name);
    return *scfg != NULL;
}

static inline bool
section_enabled(cfg_t* scfg)
{
    return cfg_getbool(scfg, "enabled") == cfg_true;
}

/**
 * @brief parses and creates a new ssl_cfg_t resource
 *
 * @param cfg the libconfuse structure for the ssl opts
 *
 * @return evhtp_ssl_cfg_t * on success, NULL on error.
 */
evhtp_ssl_cfg_t*
ssl_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    evhtp_ssl_cfg_t* scfg = ssl_cfg_new();
    if (!scfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    struct stat file_stat;
    if (cfg_getstr(cfg, "cert"))
    {
        scfg->pemfile = g_strdup(cfg_getstr(cfg, "cert"));
        if (stat(scfg->pemfile, &file_stat) != 0)
        {
            LOGE("Cannot find SSL cert file '%s'", scfg->pemfile);
            exit(EXIT_FAILURE);
        }
    }

    if (cfg_getstr(cfg, "key"))
    {
        scfg->privfile = g_strdup(cfg_getstr(cfg, "key"));
        if (stat(scfg->privfile, &file_stat) != 0)
        {
            LOGE("Cannot find SSL key file '%s'", scfg->privfile);
            exit(EXIT_FAILURE);
        }
    }

    if (cfg_getstr(cfg, "ca"))
    {
        scfg->cafile = g_strdup(cfg_getstr(cfg, "ca"));
        if (stat(scfg->cafile, &file_stat) != 0)
        {
            LOGE("Cannot find SSL ca file '%s'", scfg->cafile);
            exit(EXIT_FAILURE);
        }
    }

    if (cfg_getstr(cfg, "capath"))
    {
        scfg->capath = g_strdup(cfg_getstr(cfg, "capath"));
        if (stat(scfg->capath, &file_stat) != 0)
        {
            LOGE("Cannot find SSL capath file '%s'", scfg->capath);
            exit(EXIT_FAILURE);
        }
    }

    if (cfg_getstr(cfg, "ciphers"))
    {
        scfg->ciphers = g_strdup(cfg_getstr(cfg, "ciphers"));
        g_assert(scfg->ciphers != NULL);
    }

    if (cfg_getstr(cfg, "sni"))
    {
        scfg->sni = g_strdup(cfg_getstr(cfg, "sni"));
        LOGD("sni \"%s\"", scfg->sni);
    }

    int ssl_verify_mode = 0;
    if (cfg_getbool(cfg, "verify-peer") == cfg_true)
    {
        ssl_verify_mode |= SSL_VERIFY_PEER;
    }

    if (cfg_getbool(cfg, "enforce-peer-cert") == cfg_true)
    {
        ssl_verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }

    if (ssl_verify_mode != 0)
    {
        scfg->verify_peer        = ssl_verify_mode;
        scfg->verify_depth       = cfg_getint(cfg, "verify-depth");
        scfg->x509_verify_cb     = ssl_x509_verifyfn;
        scfg->x509_chk_issued_cb = NULL;
    }

    if (cfg_getbool(cfg, "cache-enabled") == cfg_true)
    {
        scfg->scache_type    = evhtp_ssl_scache_type_internal;
        scfg->scache_size    = cfg_getint(cfg, "cache-size");
        scfg->scache_timeout = cfg_getint(cfg, "cache-timeout");
    }

    long ssl_opts = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
    int proto_on_count = cfg_size(cfg, "protocols-on");
    for (int i = 0; i < proto_on_count; i++)
    {
        const char* proto_str = cfg_getnstr(cfg, "protocols-on", i);

        if (!strcasecmp(proto_str, "SSLv2"))
        {
            ssl_opts &= ~SSL_OP_NO_SSLv2;
        }
        else if (!strcasecmp(proto_str, "SSLv3"))
        {
            ssl_opts &= ~SSL_OP_NO_SSLv3;
        }
        else if (!strcasecmp(proto_str, "TLSv1"))
        {
            ssl_opts &= ~SSL_OP_NO_TLSv1;
        }
        else if (!strcasecmp(proto_str, "ALL"))
        {
            ssl_opts = 0;
        }
    }

    int proto_off_count = cfg_size(cfg, "protocols-off");
    for (int i = 0; i < proto_off_count; i++)
    {
        const char * proto_str = cfg_getnstr(cfg, "protocols-off", i);

        if (!strcasecmp(proto_str, "SSLv2"))
        {
            ssl_opts |= SSL_OP_NO_SSLv2;
        }
        else if (!strcasecmp(proto_str, "SSLv3"))
        {
            ssl_opts |= SSL_OP_NO_SSLv3;
        }
        else if (!strcasecmp(proto_str, "TLSv1"))
        {
            ssl_opts |= SSL_OP_NO_TLSv1;
        }
        else if (!strcasecmp(proto_str, "ALL"))
        {
            ssl_opts = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1;
        }
    }

    scfg->ssl_ctx_timeout = cfg_getint(cfg, "context-timeout");
    scfg->ssl_opts        = ssl_opts;

    cfg_t* crl_cfg;
    if (section_exists(cfg, "crl", &crl_cfg))
    {
        ssl_crl_cfg_t* crl_config;
        if (!(crl_config = g_new0(ssl_crl_cfg_t, 1)))
        {
            LOGE("Could not allocate crl cfg %s", g_strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (cfg_getstr(crl_cfg, "file"))
        {
            crl_config->filename = g_strdup(cfg_getstr(crl_cfg, "file"));

            if (stat(crl_config->filename, &file_stat) == -1 || !S_ISREG(file_stat.st_mode))
            {
                LOGE("Cannot find CRL file '%s'", crl_config->filename);
                exit(EXIT_FAILURE);
            }
        }

        if (cfg_getstr(crl_cfg, "dir"))
        {
            crl_config->dirname = g_strdup(cfg_getstr(crl_cfg, "dir"));

            if (stat(crl_config->dirname, &file_stat) != 0 || !S_ISDIR(file_stat.st_mode))
            {
                LOGE("Cannot find CRL directory '%s'", crl_config->dirname);
                exit(EXIT_FAILURE);
            }
        }

        crl_config->reload_timer.tv_sec  = cfg_getnint(crl_cfg, "reload", 0);
        crl_config->reload_timer.tv_usec = cfg_getnint(crl_cfg, "reload", 1);

        /* at the moment evhtp does not give us an area where we can store this
         * type of information without breaking the configuration structure. But
         * it does have an optional user-supplied arguments, which we use here
         * to store our CRL configuration.
         */
        scfg->args = (void *)crl_config;
    }

    return scfg;
} /* ssl_cfg_parse */

/**
 * @brief parses ssl x509 extension headers
 *
 * @param cfg
 *
 * @return
 */
x509_ext_cfg_t*
x509_ext_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    x509_ext_cfg_t* x509cfg = x509_ext_cfg_new();
    if (!x509cfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    char* name =cfg_getstr(cfg, "name");
    if (name)
    {
        x509cfg->name = g_strdup(name);
        g_assert(x509cfg->name != NULL);
    }

    char* oid = cfg_getstr(cfg, "oid");
    if (oid)
    {
        x509cfg->oid = g_strdup(oid);
        g_assert(x509cfg->oid);
    }

    LOGD("x509cfg %p", x509cfg);
    return x509cfg;
}

/**
 * @brief parses header addition config from server vhost { { rules { rule { headers { } } } } }
 *
 * @param cfg
 *
 * @return
 */
headers_cfg_t*
headers_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    headers_cfg_t* hcfg = headers_cfg_new();
    if (!hcfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    hcfg->x_forwarded_for   = cfg_getbool(cfg, "x-forwarded-for");
    hcfg->x_ssl_subject     = cfg_getbool(cfg, "x-ssl-subject");
    hcfg->x_ssl_issuer      = cfg_getbool(cfg, "x-ssl-issuer");
    hcfg->x_ssl_notbefore   = cfg_getbool(cfg, "x-ssl-notbefore");
    hcfg->x_ssl_notafter    = cfg_getbool(cfg, "x-ssl-notafter");
    hcfg->x_ssl_sha1        = cfg_getbool(cfg, "x-ssl-sha1");
    hcfg->x_ssl_serial      = cfg_getbool(cfg, "x-ssl-serial");
    hcfg->x_ssl_cipher      = cfg_getbool(cfg, "x-ssl-cipher");
    hcfg->x_ssl_certificate = cfg_getbool(cfg, "x-ssl-certificate");

    int n_x509_exts = cfg_size(cfg, "x509-extension");
    for (int i = 0; i < n_x509_exts; i++)
    {
        x509_ext_cfg_t* x509cfg = x509_ext_cfg_parse(cfg_getnsec(cfg, "x509-extension", i));
        g_assert(x509cfg != NULL);

        lztq_elem* elem = lztq_append(hcfg->x509_exts, x509cfg, sizeof(x509cfg), x509_ext_cfg_free);
        g_assert(elem != NULL);
    }

    LOGD("hcfg %p", hcfg);
    return hcfg;
}

static bool
do_headers_section(cfg_t* cfg, headers_cfg_t** hdr_cfg)
{
    LOGD("(%p, %p)", cfg, hdr_cfg);

    g_return_val_if_fail(cfg != NULL, false);
    g_return_val_if_fail(hdr_cfg != NULL, false);

    *hdr_cfg = NULL;

    cfg_t* scfg;
    if (!section_exists(cfg, "headers", &scfg))
    {
        LOGD("no headers section");
    }
    else
    {
        headers_cfg_t* hdr_cfg_ = headers_cfg_parse(scfg);
        if (!hdr_cfg_)
        {
            LOGE("parse failed");
            return false;
        }
        *hdr_cfg = hdr_cfg_;
    }
    return true;
}

/**
 * @brief parses a single rule from a server { vhost { rules { } } } config
 *
 * @param cfg
 *
 * @return
 */
rule_cfg_t*
rule_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    rule_cfg_t* rcfg = rule_cfg_new();
    if (!rcfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    const char* rname = cfg_title(cfg);
    g_assert(rname != NULL);

    rcfg->name = g_strdup(rname);
    g_assert(rcfg->name != NULL);

    if (cfg_getstr(cfg, "uri-match"))
    {
        rcfg->type     = rule_type_exact;
        rcfg->matchstr = g_strdup(cfg_getstr(cfg, "uri-match"));
    }
    else if (cfg_getstr(cfg, "uri-gmatch"))
    {
        rcfg->type     = rule_type_glob;
        rcfg->matchstr = g_strdup(cfg_getstr(cfg, "uri-gmatch"));
    }
    else if (cfg_getstr(cfg, "uri-rmatch"))
    {
        rcfg->type     = rule_type_regex;
        rcfg->matchstr = g_strdup(cfg_getstr(cfg, "uri-rmatch"));
    }
    else
    {
        // Match "anything" by default.
        rcfg->type = rule_type_default;
    }

    rcfg->lb_method      = lbstr_to_lbtype(cfg_getstr(cfg, "lb-method"));
    rcfg->discovery_type = discovery_type_str_to_discovery_type(cfg_getstr(cfg, "discovery-type"));
    if (!do_headers_section(cfg, &rcfg->headers))
    {
        LOGE("header section failed");
        rule_cfg_free(rcfg);
        return NULL;
    }
    rcfg->passthrough    = cfg_getbool(cfg, "passthrough");
    rcfg->allow_redirect = cfg_getbool(cfg, "allow-redirect");

    if (cfg_getopt(cfg, "upstream-read-timeout"))
    {
        rcfg->up_read_timeout.tv_sec  = cfg_getnint(cfg, "upstream-read-timeout", 0);
        rcfg->up_read_timeout.tv_usec = cfg_getnint(cfg, "upstream-read-timeout", 1);
        rcfg->has_up_read_timeout     = 1;
    }

    if (cfg_getopt(cfg, "upstream-write-timeout"))
    {
        rcfg->up_write_timeout.tv_sec  = cfg_getnint(cfg, "upstream-write-timeout", 0);
        rcfg->up_write_timeout.tv_usec = cfg_getnint(cfg, "upstream-write-timeout", 1);
        rcfg->has_up_write_timeout     = 1;
    }

    int n_upstreams = cfg_size(cfg, "upstreams");
    for (int i = 0; i < n_upstreams; i++)
    {
        char* ds_name = cfg_getnstr(cfg, "upstreams", i);
        if (!ds_name)
        {
            LOGE("upstream must have a name");
            rule_cfg_free(rcfg);
            return NULL;
        }
        ds_name = g_strdup(ds_name);
        if (!ds_name)
        {
            LOGE("alloc failed");
            rule_cfg_free(rcfg);
            return NULL;
        }

        lztq_elem* elem = lztq_append(rcfg->upstreams, ds_name, strlen(ds_name), g_free);
        g_assert(elem != NULL);
    }

    int n_filters;
    if (rcfg->allow_redirect != 0 && ((n_filters = cfg_size(cfg, "redirect-filter")) > 0))
    {
        /*
         * if the redirect option is enabled, optionally an administrator can
         * add a list of allowed hosts it may communicate with.
         */
        rcfg->redirect_filter = lztq_new();
        g_assert(rcfg->redirect_filter != NULL);

        for (int i = 0; i < n_filters; i++)
        {
            char* host_ent = g_strdup(cfg_getnstr(cfg, "redirect-filter", i));
            g_assert(host_ent != NULL);

            lztq_elem* elem = lztq_append(rcfg->redirect_filter, host_ent,
                                   strlen(host_ent), g_free);
            g_assert(elem != NULL);
        }
    }

    LOGD("rcfg %p", rcfg);
    return rcfg;
} /* rule_cfg_parse */

static bool
do_ssl_section(cfg_t* cfg, evhtp_ssl_cfg_t** ssl_cfg)
{
    LOGD("(%p, %p)", cfg, ssl_cfg);

    g_return_val_if_fail(cfg != NULL, false);
    g_return_val_if_fail(ssl_cfg != NULL, false);

    *ssl_cfg = NULL;

    cfg_t* scfg;
    if (!section_exists(cfg, "ssl", &scfg))
    {
        LOGD("no ssl section");
    }
    else if (!section_enabled(scfg))
    {
        LOGD("ssl disabled");
    }
    else
    {
        evhtp_ssl_cfg_t* ssl_cfg_ = ssl_cfg_parse(scfg);
        if (!ssl_cfg_)
        {
            LOGE("parse failed");
            return false;
        }
        *ssl_cfg = ssl_cfg_;
    }
    return true;
}

/**
 * @brief parses a upstream {} config entry from a server { } config.
 *
 * @param cfg
 *
 * @return
 */
upstream_cfg_t*
upstream_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    upstream_cfg_t* dscfg = upstream_cfg_new();
    if (!dscfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    dscfg->name                  = g_strdup(cfg_title(cfg));
    dscfg->enabled               = cfg_getbool(cfg, "enabled");
    dscfg->host                  = g_strdup(cfg_getstr(cfg, "addr"));
    dscfg->port                  = cfg_getint(cfg, "port");
    dscfg->n_connections         = cfg_getint(cfg, "connections");
    dscfg->high_watermark        = cfg_getint(cfg, "high-watermark");

    dscfg->read_timeout.tv_sec   = cfg_getnint(cfg, "read-timeout", 0);
    dscfg->read_timeout.tv_usec  = cfg_getnint(cfg, "read-timeout", 1);
    dscfg->write_timeout.tv_sec  = cfg_getnint(cfg, "write-timeout", 0);
    dscfg->write_timeout.tv_usec = cfg_getnint(cfg, "write-timeout", 1);
    dscfg->retry_ival.tv_sec     = cfg_getnint(cfg, "retry", 0);
    dscfg->retry_ival.tv_usec    = cfg_getnint(cfg, "retry", 1);

    if (!do_ssl_section(cfg, &dscfg->ssl_cfg))
    {
        LOGE("ssl section failed");
        upstream_cfg_free(dscfg);
        return NULL;
    }

    if (dscfg->enabled == true)
    {
        _rusage.total_num_connections += dscfg->n_connections;
    }

    LOGD("dscfg %p", dscfg);
    return dscfg;
}

static bool
do_logger_section(cfg_t* cfg, const char* type, logger_cfg_t** log_cfg)
{
    LOGD("(%p, %p(%s), %p)", cfg, type, type, log_cfg);

    g_return_val_if_fail(cfg != NULL, false);
    g_return_val_if_fail(type != NULL, false);
    g_return_val_if_fail(type[0] != '\0', false);
    g_return_val_if_fail(log_cfg != NULL, false);

    *log_cfg = NULL;

    cfg_t* scfg;
    if (!section_exists(cfg, type, &scfg))
    {
        LOGD("no %s section", type);
    }
    else if (!section_enabled(scfg))
    {
        LOGD("%s not enabled", type);
    }
    else
    {
        logger_cfg_t* log_cfg_ = logger_cfg_parse(scfg);
        if (!log_cfg_)
        {
            LOGE("parse failed");
            return false;
        }
        *log_cfg = log_cfg_;
    }
    return true;
}


vhost_cfg_t*
vhost_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    vhost_cfg_t* vcfg = vhost_cfg_new();
    if (!vcfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    vcfg->server_name = g_strdup(cfg_title(cfg));
    g_assert(vcfg->server_name != NULL);
    LOGD("server_name %p(%s)", vcfg->server_name, vcfg->server_name);

    if (!do_ssl_section(cfg, &vcfg->ssl_cfg))
    {
        LOGE("ssl section failed");
        vhost_cfg_free(vcfg);
        return NULL;
    }

    for (int i = 0; i < cfg_size(cfg, "rule"); i++)
    {
        rule_cfg_t* rule_cfg;
        if (!(rule_cfg = rule_cfg_parse(cfg_getnsec(cfg, "rule", i))))
        {
            LOGE("parse failed");
            return NULL;
        }

        lztq_elem* elem = lztq_append(vcfg->rule_cfgs, rule_cfg, sizeof(rule_cfg), rule_cfg_free);
        g_assert(elem != NULL);
    }

    for (int i = 0; i < cfg_size(cfg, "aliases"); i++)
    {
        g_assert(cfg_getnstr(cfg, "aliases", i) != NULL);

        char* name = g_strdup(cfg_getnstr(cfg, "aliases", i));
        g_assert(name != NULL);

        lztq_elem* elem = lztq_append(vcfg->aliases, name, strlen(name), g_free);
        g_assert(elem != NULL);
    }

    if (cfg_size(cfg, "strip-headers"))
    {
        vcfg->strip_hdrs = lztq_new();
        g_assert(vcfg->strip_hdrs != NULL);

        for (int i = 0; i < cfg_size(cfg, "strip-headers"); i++)
        {
            g_assert(cfg_getnstr(cfg, "strip-headers", i) != NULL);

            char* hdr_name = g_strdup(cfg_getnstr(cfg, "strip-headers", i));
            g_assert(hdr_name != NULL);

            lztq_elem* elem = lztq_append(vcfg->strip_hdrs, hdr_name, strlen(hdr_name), g_free);
            g_assert(elem != NULL);
        }
    }

    if (cfg_size(cfg, "rewrite-urls"))
    {
        vcfg->rewrite_urls = lztq_new();
        g_assert(vcfg->rewrite_urls != NULL);

        for (int i = 0; i < cfg_size(cfg, "rewrite-urls"); i++)
        {
            g_assert(cfg_getnstr(cfg, "rewrite-urls", i) != NULL);

            char* rewrite_url = g_strdup(cfg_getnstr(cfg, "rewrite-urls", i));
            g_assert(rewrite_url != NULL);

            lztq_elem* elem = lztq_append(vcfg->rewrite_urls, rewrite_url, strlen(rewrite_url), g_free);
            g_assert(elem != NULL);
        }
    }

    cfg_t* log_cfg;
    if (section_exists(cfg, "logging", &log_cfg))
    {
        if (!do_logger_section(log_cfg, "request", &vcfg->req_log) ||
            !do_logger_section(log_cfg, "error", &vcfg->err_log))
        {
            LOGE("logger section failed");
            vhost_cfg_free(vcfg);
            return NULL;
        }
    }

    if (!do_headers_section(cfg, &vcfg->headers))
    {
        LOGE("headers section failed");
        vhost_cfg_free(vcfg);
        return NULL;
    }

    LOGD("vcfg %p", vcfg);
    return vcfg;
} /* vhost_cfg_parse */

/**
 * @brief parses a server {} entry from a config.
 *
 * @param cfg
 *
 * @return
 */
server_cfg_t*
server_cfg_parse(cfg_t* cfg)
{
    LOGD("(%p)", cfg);

    g_return_val_if_fail(cfg != NULL, NULL);

    server_cfg_t* scfg = server_cfg_new();
    if (!scfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    scfg->name = g_strdup(cfg_title(cfg));
    LOGD("server \"%s\"", scfg->name);

    scfg->bind_addr               = g_strdup(cfg_getstr(cfg, "addr"));
    scfg->bind_port               = cfg_getint(cfg, "port");
    if (!do_ssl_section(cfg, &scfg->ssl_cfg))
    {
        LOGE("ssl section failed");
        server_cfg_free(scfg);
        return NULL;
    }
    scfg->num_threads             = cfg_getint(cfg, "threads");
    scfg->listen_backlog          = cfg_getint(cfg, "backlog");
    scfg->max_pending             = cfg_getint(cfg, "max-pending");
    scfg->read_timeout.tv_sec     = cfg_getnint(cfg, "read-timeout", 0);
    scfg->read_timeout.tv_usec    = cfg_getnint(cfg, "read-timeout", 1);
    scfg->write_timeout.tv_sec    = cfg_getnint(cfg, "write-timeout", 0);
    scfg->write_timeout.tv_usec   = cfg_getnint(cfg, "write-timeout", 1);
    scfg->pending_timeout.tv_sec  = cfg_getnint(cfg, "pending-timeout", 0);
    scfg->pending_timeout.tv_usec = cfg_getnint(cfg, "pending-timeout", 1);
    scfg->high_watermark          = cfg_getint(cfg, "high-watermark");

    if (cfg_getbool(cfg, "disable-server-nagle") == cfg_true)
    {
        LOGD("disable server nagle");
        scfg->disable_server_nagle = true;
    }

    if (cfg_getbool(cfg, "disable-client-nagle") == cfg_true)
    {
        LOGD("disabled client nagle");
        scfg->disable_client_nagle = true;
    }

    if (cfg_getbool(cfg, "disable-upstream-nagle") == cfg_true)
    {
        LOGD("disable upstream nagle");
        scfg->disable_upstream_nagle = true;
    }

    if (cfg_getbool(cfg, "enable-workers-listen") == cfg_true)
    {
        LOGD("enable workers listen");
        scfg->enable_workers_listen = true;
    }

    cfg_t* log_cfg;
    if (section_exists(cfg, "logging", &log_cfg))
    {
        if (!do_logger_section(log_cfg, "request", &scfg->req_log_cfg) ||
            !do_logger_section(log_cfg, "error", &scfg->err_log_cfg))
        {
            LOGE("logger section failed");
            server_cfg_free(scfg);
            return NULL;
        }
    }

    /* parse and insert all the configured upstreams */
    for (int i = 0; i < cfg_size(cfg, "upstream"); i++)
    {
        upstream_cfg_t* dscfg = upstream_cfg_parse(cfg_getnsec(cfg, "upstream", i));
        g_assert(dscfg != NULL);

        lztq_elem* elem = lztq_append(scfg->upstreams, dscfg, sizeof(dscfg), upstream_cfg_free);
        g_assert(elem != NULL);
    }

    for (int i = 0; i < cfg_size(cfg, "vhost"); i++)
    {
        vhost_cfg_t* vcfg = vhost_cfg_parse(cfg_getnsec(cfg, "vhost", i));
        g_assert(vcfg != NULL);

        lztq_elem* elem = lztq_append(scfg->vhosts, vcfg, sizeof(vcfg), vhost_cfg_free);
        g_assert(elem != NULL);
    }

    if (scfg->num_threads <= 0) scfg->num_threads = g_get_num_processors();

    _rusage.total_num_threads += scfg->num_threads;
    _rusage.total_max_pending += scfg->max_pending;

    cfg_t* rule_cfg;
    if (section_exists(cfg, "rule", &rule_cfg))
    {
        LOGD("default rule");
        scfg->default_rule_cfg = rule_cfg_parse(rule_cfg);
    }

    LOGD("scfg %p", scfg);
    return scfg;
} /* server_cfg_parse */

static rproxy_cfg_t*
rproxy_cfg_parse_(cfg_t* cfg, const char* filename)
{
    LOGD("(%p, %p(%s))", cfg, filename, filename);

    g_return_val_if_fail(cfg != NULL, NULL);

    rproxy_cfg_t* rpcfg = rproxy_cfg_new(filename);
    if (!rpcfg)
    {
        LOGE("alloc failed");
        return NULL;
    }

    char* user = cfg_getstr(cfg, "user");
    if (user)
    {
        rpcfg->user = g_strdup(user);
        g_assert(rpcfg->user != NULL);
    }

    char* group = cfg_getstr(cfg, "group");
    if (group)
    {
        rpcfg->group = g_strdup(group);
        g_assert(rpcfg->group != NULL);
    }

    char* rootdir = cfg_getstr(cfg, "rootdir");
    if (rootdir)
    {
        rpcfg->rootdir = g_strdup(rootdir);
        g_assert(rpcfg->rootdir != NULL);
    }

    rpcfg->max_nofile = cfg_getint(cfg, "max-nofile");
    rpcfg->daemonize  = cfg_getbool(cfg, "daemonize");

    int n_servers = cfg_size(cfg, "server");
    for (int i = 0; i < n_servers; i++)
    {
        server_cfg_t* scfg = server_cfg_parse(cfg_getnsec(cfg, "server", i));
        g_assert(scfg != NULL);

        scfg->rproxy_cfg = rpcfg;

        lztq_elem* elem = lztq_append(rpcfg->servers, scfg, sizeof(scfg), server_cfg_free);
        g_assert(elem != NULL);
    }

    /* set our rusage settings from the global one */
    memcpy(&rpcfg->rusage, &_rusage, sizeof(rproxy_rusage_t));

    LOGD("rpcfg %p", rpcfg);
    return rpcfg;
} /* rproxy_cfg_parse_ */

rproxy_cfg_t*
rproxy_cfg_parse(const char* filename)
{
    LOGD("(%p(%s))", filename, filename);

    g_return_val_if_fail(filename != NULL, NULL);
    g_return_val_if_fail(filename[0] != '\0', NULL);

    cfg_t* cfg;
    if (!(cfg = cfg_init(rproxy_opts, CFGF_NOCASE|CFGF_IGNORE_UNKNOWN)))
    {
        LOGE("cfg_init() failed");
        return NULL;
    }

    if (cfg_parse(cfg, filename) != 0)
    {
        int err = errno;
        LOGE("cfg_parse() failed, errno %d(%s)", err, g_strerror(err));
        cfg_free(cfg);
        return NULL;
    }

    rproxy_cfg_t* rp_cfg = rproxy_cfg_parse_(cfg, filename);
    cfg_free(cfg);

    LOGD("rp_cfg %p", rp_cfg);
    return rp_cfg;
}
