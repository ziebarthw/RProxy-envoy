#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>

#ifndef NO_RLIMITS
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#if (defined(util_NOISY) || defined(ALL_NOISY)) && !defined(NO_util_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "utils/header_value_parser.h"
#include "rproxy.h"
#include "rp-headers.h"
#include "gbrotlidecompressor.h"
#include "gpassthroughconverter.h"

int
util_write_header_to_evbuffer(evhtp_header_t* hdr, void* arg)
{
    LOGD("(%p, %p)", hdr, arg);

    evbuf_t* buf = arg;
    g_assert(buf != NULL);

    evbuffer_add(buf, hdr->key, hdr->klen);
    evbuffer_add(buf, ": ", 2);
    evbuffer_add(buf, hdr->val, hdr->vlen);
    evbuffer_add(buf, "\r\n", 2);

    return 0;
}

evbuf_t*
util_request_to_evbuffer(evhtp_request_t* r, evbuf_t* obuf)
{
    LOGD("(%p, %p)", r, obuf);

    g_return_val_if_fail(r != NULL, NULL);

    evbuf_t* buf = obuf ? obuf : evbuffer_new();
    if (!buf)
    {
        LOGE("alloc failed");
        return NULL;
    }

    const char* query_args = (r->uri && r->uri->query_raw) ? (const char*)r->uri->query_raw : "";
    if (query_args[0] == '?') ++query_args;

    evbuffer_add_printf(buf, "%s %s%s%s HTTP/%d.%d\r\n",
                        htparser_get_methodstr(r->conn->parser),
                        r->uri->path->full,
                        *query_args ? "?" : "", query_args,
                        htparser_get_major(r->conn->parser),
                        htparser_get_minor(r->conn->parser));

    evhtp_headers_for_each(r->headers_in, util_write_header_to_evbuffer, buf);
    evbuffer_add(buf, "\r\n", 2);

    return buf;
}

static inline const char*
evhtp_request_get_proto_str(evhtp_request_t* request)
{
    switch (evhtp_request_get_proto(request))
    {
        case EVHTP_PROTO_11:
            return "1.1";
        case EVHTP_PROTO_10:
            return "1.0";
        default:
            return "0.9";
    }
}

evbuf_t*
util_response_to_evbuffer(evhtp_request_t* r, evbuf_t* obuf)
{
    LOGD("(%p, %p)", r, obuf);

    g_return_val_if_fail(r != NULL, NULL);

    evbuf_t* buf = obuf ? obuf : evbuffer_new();
    if (!buf)
    {
        LOGE("alloc failed");
        return NULL;
    }

    evbuffer_add_printf(obuf, "HTTP/%s %d %s\r\n",
                        evhtp_request_get_proto_str(r),
                        evhtp_request_get_status_code(r),
                        evhtp_request_get_status_code_str(r));

    evhtp_headers_for_each(r->headers_out, util_write_header_to_evbuffer, buf);
    evbuffer_add(buf, "\r\n", 2);

    return buf;
}

void
util_dropperms(const char* user, const char* group)
{
    LOGD("(%p(%s), %p(%s))", user, user, group, group);

    if (group)
    {
        struct group* grp;
        if (!(grp = getgrnam(group)))
        {
            LOGE("No such group '%s'", group);
            exit(1);
        }

        if (setgid(grp->gr_gid) != 0)
        {
            gint errnum = errno;
            LOGE("Could not grp perm to '%s' (%s)", group, g_strerror(errnum));
            exit(1);
        }
    }

    if (user)
    {
        struct passwd* usr;
        if (!(usr = getpwnam(user)))
        {
            LOGE("No such user '%s'", user);
            exit(1);
        }

        if (seteuid(usr->pw_uid) != 0)
        {
            gint errnum = errno;
            LOGE("Could not usr perm to '%s' (%s)", user, g_strerror(errnum));
            exit(1);
        }
    }
}

int
util_daemonize(char* root, int noclose)
{
    LOGD("(%p(%s), %d)", root, root, noclose);

    switch (fork())
    {
        case -1:
            return -1;
        case 0:
            break;
        default:
            exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
    {
        LOGD("setsid() == -1");
        return -1;
    }

// REVISIT: ???????
    if (root == 0)
    {
        if (chdir(root) != 0)
        {
            perror("chdir");
            return -1;
        }
    }

    int fd;
    if (noclose == 0 && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 stdin");
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            return -1;
        }
        if (dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dup2 stderr");
            return -1;
        }

        if (fd > STDERR_FILENO)
        {
            if (close(fd) < 0)
            {
                perror("close");
                return -1;
            }
        }
    }
    return 0;
} /* daemonize */

int
util_set_rlimits(int nofiles)
{
    LOGD("(%d)", nofiles);

#ifndef NO_RLIMITS
    struct rlimit limit;
//    rlim_t        max_nofiles;

    if (nofiles <= 0) {
        return -1;
    }

    if (getrlimit(RLIMIT_NOFILE, &limit) == -1) {
        g_error("Could not obtain curr NOFILE lim: %s\n", strerror(errno));
        return 0;
    }

    if (nofiles > limit.rlim_max) {
        g_error("Unable to set curr NOFILE (requested=%d, sys-limit=%d)\n",
                (int)nofiles, (int)limit.rlim_max);
        g_error("Please make sure your systems limits.conf is set high enough (usually in /etc/security/limits.conf!\n");
        return -1;
    }

    if (nofiles < 10000) {
        g_warning("%d max-nofiles is very small, this could be bad, lets check...", nofiles);

        if ((int)limit.rlim_max >= 10000) {
            g_info("using %d (your hard-limit) on max-nofiles instead of %d!", (int)limit.rlim_max, nofiles);
            nofiles = limit.rlim_max;
        } else {
            g_warning("nope, can't go any higher, you may want to fix this...");
        }
    }

    limit.rlim_cur = nofiles;

    if (setrlimit(RLIMIT_NOFILE, &limit) == -1) {
        g_error("Could not set NOFILE lim: %s\n", strerror(errno));
        return -1;
    }

#else
    LOGE("Your system does not support get|setrlimits!");
#endif
    return 0;
} /* rproxy_set_rlimits */

/**
 * @brief glob/wildcard type pattern matching.
 *
 * Note: This code was derived from redis's (v2.6) stringmatchlen() function.
 *
 * @param pattern
 * @param string
 *
 * @return true if matches, false if not.
 */
bool
util_glob_match(const char* pattern, const char* string)
{
    LOGD("(%p(%s), %p(%s))", pattern, pattern, string, string);

    if (!pattern)
    {
        LOGD("pattern is null");
    }
    else if (!string)
    {
        LOGD("string is null");
    }
    else
    {
        size_t pat_len = strlen(pattern);
        size_t str_len = strlen(string);

        while (pat_len)
        {
            if (pattern[0] == '*')
            {
                while (pattern[1] == '*')
                {
                    pattern++;
                    pat_len--;
                }

                if (pat_len == 1)
                {
                    LOGD("match");
                    return true;
                }

                while (str_len)
                {
                    if (util_glob_match(pattern + 1, string))
                    {
                        LOGD("match");
                        return true;
                    }

                    string++;
                    str_len--;
                }

                LOGD("no match");
                return false;
            }
            else
            {
                if (pattern[0] != string[0])
                {
                    LOGD("no match");
                    return false;
                }

                string++;
                str_len--;
            }

            pattern++;
            pat_len--;

            if (str_len == 0)
            {
                while (*pattern == '*')
                {
                    pattern++;
                    pat_len--;
                }
                break;
            }
        }

        if (pat_len == 0 && str_len == 0)
        {
            LOGD("match");
            return true;
        }
    }

    LOGD("no match");
    return false;
} /* util_glob_match */

static int
_glob_match_iterfn(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    const char* needle = arg;
    if (!needle)
    {
        LOGD("needle is null");
        return -1;
    }

    const char* haystack = lztq_elem_data(elem);
    if (!haystack)
    {
        LOGD("haystack is null");
        return -1;
    }

    return util_glob_match(haystack, needle);
}

/**
 * @brief iterate over a tailq of strings and attempt to find a wildcard match.
 *
 * @param tq a lztq of strings to match against. (haystack)
 * @param str  the string to compare. (needle)
 *
 * @return -1 on error, 1 on match, 0 on no match.
 */
bool
util_glob_match_lztq(lztq* tq, const char* str)
{
    LOGD("(%p, %p(%s))", tq, str, str);

    /* if the tailq is empty, we default to allowed */
    if (tq == NULL || lztq_size(tq) == 0)
    {
        LOGD("yep");
        return true;
    }

    /* since lztq_for_each will break from the loop if the iterator returns
     * anything but zero, we utilize the result as an indicator as to whether
     * the wildcard match was sucessful.
     *
     * result =  0 == no match
     * result =  1 == match
     * result = -1 == matching error
     */
    return lztq_for_each(tq, _glob_match_iterfn, (void*)str) == true;
}

static int
_rm_headers_iterfn(lztq_elem* elem, void* arg)
{
    LOGD("(%p, %p)", elem, arg);

    evhtp_headers_t* headers = arg;
    if (!headers)
    {
        LOGD("headers is null");
        return -1;
    }

    const char* header_key = lztq_elem_data(elem);
    if (!header_key)
    {
        LOGD("header_key is null");
        return -1;
    }

    evhtp_kv_rm_and_free(headers, evhtp_kvs_find_kv(headers, header_key));

    return 0;
}

/**
 * @brief iterates over a lzq of strings and removes any headers with that
 *        string.
 *
 * @param tq
 * @param headers
 *
 * @return 0 on success, otherwise -1
 */
int
util_rm_headers_via_lztq(lztq * tq, evhtp_headers_t * headers)
{
    LOGD("(%p, %p)", tq, headers);

    if (!tq)
    {
        LOGD("tq is null");
    }
    else if (!headers)
    {
        LOGD("headers is null");
    }
    else
    {
        return lztq_for_each(tq, _rm_headers_iterfn, (void *)headers);
    }
    return 0;
}

// Taken from libevent util; for some reason they did not expose it to the
// outside world. Not sure why.(?)
const char*
util_format_sockaddr_port(const struct sockaddr* sa, char* out, size_t outlen)
{
    NOISY_MSG_("(%p, %p, %zu)", sa, out, outlen);
    char buf[128];
    const char* res = NULL;
    int port;
    if (sa->sa_family == AF_INET)
    {
        const struct sockaddr_in* sin = (const struct sockaddr_in*)sa;
        res = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
        port = ntohs(sin->sin_port);
        if (res)
        {
            UTIL_IS_DEFAULT_PORT(port) ?
                evutil_snprintf(out, outlen, "%s", buf) :
                evutil_snprintf(out, outlen, "%s:%d", buf, port);
            return out;
        }
    }
    else if (sa->sa_family == AF_INET6)
    {
        const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)sa;
        res = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf));
        port = ntohs(sin6->sin6_port);
        if (res)
        {
            UTIL_IS_DEFAULT_PORT(port) ?
                evutil_snprintf(out, outlen, "%s", buf) :
                evutil_snprintf(out, outlen, "[%s]:%d", buf, port);
            return out;
        }
    }
    return ":";
}

static inline const char*
util_htp_scheme_tostr(htp_scheme scheme)
{
    switch (scheme)
    {
        case htp_scheme_ftp:
            return "ftp";
        case htp_scheme_http:
            return "http";
        case htp_scheme_https:
            return "https";
        case htp_scheme_nfs:
            return "nfs";
        case htp_scheme_unknown:
        case htp_scheme_none:
        default:
            return NULL;
    }
}

static inline const char*
get_host_hdr_value(const evhtp_request_t* req)
{
    LOGD("(%p)", req);
    return evhtp_header_find(req->headers_in, RpHeaderValues.HostLegacy);
}

static inline const char*
get_hostname(const evhtp_request_t* req, const char* hostname)
{
    LOGD("(%p, %p(%s))", req, hostname, hostname);
    if (hostname && hostname[0])
    {
        LOGD("explicit hostname %p(%s) provided", hostname, hostname);
        return hostname;
    }
    hostname = req->uri->authority->hostname;
    if (hostname)
    {
        LOGD("authority hostname %p(%s)", hostname, hostname);
        return hostname;
    }
    hostname = get_host_hdr_value(req);
    if (hostname)
    {
        LOGD("host header %p(%s)", hostname, hostname);
        return hostname;
    }
    LOGD("localhost");
    return "localhost";
}

bool
util_uri_to_evbuffer(const evhtp_request_t* req, const char* hostname, evbuf_t* obuf)
{
    LOGD("(%p, %p(%s), %p)", req, hostname, hostname, obuf);

    g_return_val_if_fail(req != NULL, false);
    g_return_val_if_fail(req->uri != NULL, false);
    g_return_val_if_fail(obuf != NULL, false);

    hostname = get_hostname(req, hostname);
    LOGD("hostname %p(%s)", hostname, hostname);

    evhtp_uri_t* uri = req->uri;
    const char* scheme = uri->scheme ?
        util_htp_scheme_tostr(uri->scheme) :
            req->conn->ssl ? "https" : "http";
    LOGD("scheme %p(%s)", scheme, scheme);

    char* query_args = uri->query_raw ? (char*)uri->query_raw : "";
    LOGD("query_args \"%s\"", query_args);

    char* fragment_args = uri->fragment ? (char*)uri->fragment : "";
    LOGD("fragment_args \"%s\"", fragment_args);

    evbuffer_add_printf(obuf, "%s://%s%s%s%s%s%s",
        scheme,
        hostname,
        uri->path->full,
        query_args[0] ? "?" : "",
        query_args,
        fragment_args[0] ? "#" : "",
        fragment_args);

    LOGD("\"%s\"", evbuffer_pullup(obuf, -1));
    return true;
}

bool
util_filename_is_uri(const char* filename)
{
    g_autofree char* scheme = g_uri_parse_scheme(filename);
    return scheme && g_str_has_prefix(scheme, "http");
}

bool
util_is_text_content_type(const char* value)
{
    LOGD("(%p(%s))", value, value);

    bool rval = false;
    if (value && value[0])
    {
        struct tokenizer_cursor_s cursor = tokenizer_cursor_ctor(0, strlen(value));
        struct header_value_parser_s hvp = header_value_parser_ctor();
        struct header_element_s he = header_value_parser_parse_header_element(&hvp, value, &cursor);
        char* type = header_element_get_name(&he);
        NOISY_MSG_("type %p(%s)", type, type);
        rval = g_str_has_prefix(type, "text/") ||
                g_str_has_suffix(type, "json") ||
                g_str_has_suffix(type, "javascript") ||
                g_str_has_suffix(type, "ecmascript") ||
                g_str_has_suffix(type, "xml");
        header_element_dtor(&he);
    }
    NOISY_MSG_("rval %u", rval);
    return rval;
}

enc_type
util_get_enc_type(const char* enc_value)
{
    LOGD("(%p(%s))", enc_value, enc_value);

    enc_type rval = enc_type_none;
    if (!enc_value)
    {
        LOGD("none");
    }
    else if (!enc_value[0])
    {
        LOGD("none");
    }
    else if (g_ascii_strcasecmp(enc_value, RpCustomHeaderValues.ContentEncodingValues.Gzip) == 0)
    {
        LOGD("gzip");
        rval = enc_type_gzip;
    }
    else if (g_ascii_strcasecmp(enc_value, RpCustomHeaderValues.ContentEncodingValues.Brotli) == 0)
    {
        LOGD("br");
        rval = enc_type_br;
    }
    else if (g_ascii_strcasecmp(enc_value, RpCustomHeaderValues.ContentEncodingValues.Deflate) == 0)
    {
        LOGD("deflate");
        rval = enc_type_deflate;
    }
    else if (g_ascii_strcasecmp(enc_value, "x-gzip") == 0)
    {
        LOGD("x-gzip");
        rval = enc_type_gzip;
    }
    return rval;
}

GInputStream*
util_create_input_stream(GInputStream* base, enc_type type)
{
    LOGD("(%p, %d)", base, type);

    g_return_val_if_fail(base != NULL, NULL);

    g_autoptr(GConverter) converter = NULL;

    switch (type)
    {
        case enc_type_gzip:
            converter = G_CONVERTER(g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP));
            break;
        case enc_type_br:
            converter = G_CONVERTER(g_brotli_decompressor_new());
            break;
        case enc_type_deflate:
            converter = G_CONVERTER(g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB));
            break;
        default:
            converter = G_CONVERTER(g_pass_through_converter_new());
            break;
    }

    return g_converter_input_stream_new(base, converter);
}
