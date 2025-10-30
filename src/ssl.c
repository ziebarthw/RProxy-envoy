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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <glib.h>

#ifndef ML_LOG_LEVEL
#define ML_LOG_LEVEL 4
#endif
#include "macrologger.h"

#include "rproxy.h"

unsigned char*
ssl_subject_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    char          * p    = NULL;
    X509          * cert = NULL;
    X509_NAME     * name;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(name = X509_get_subject_name(cert)))
    {
        LOGE("alloc failed");
    }
    else if (!(p = X509_NAME_oneline(name, NULL, 0)))
    {
        LOGE("alloc failed");
    }
    else
    {
        unsigned char * subj_str = (unsigned char*)g_strdup(p);

        OPENSSL_free(p);
        X509_free(cert);

        return subj_str;
    }

    g_clear_pointer(&cert, X509_free);
    if (p) OPENSSL_free(p);

    LOGD("failed");
    return NULL;
}

unsigned char*
ssl_issuer_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    X509* cert = NULL;
    X509_NAME* name;
    char* p = NULL;
    unsigned char* issr_str;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(name = X509_get_issuer_name(cert)))
    {
        LOGD("issuer name is null");
    }
    else if (!(p = X509_NAME_oneline(name, NULL, 0)))
    {
        LOGE("alloc failed");
    }
    else if (!(issr_str = (unsigned char*)g_strdup(p)))
    {
        LOGE("alloc failed");
    }
    else
    {
        OPENSSL_free(p);
        X509_free(cert);
        return issr_str;
    }

    g_clear_pointer(&cert, X509_free);
    if (p) OPENSSL_free(p);

    LOGD("failed");
    return NULL;
}

static unsigned char*
ssl_time_tostr(evhtp_ssl_t* ssl, ASN1_TIME*(*X509_time_f)(const X509*))
{
    LOGD("(%p)", ssl);

    BIO           * bio = NULL;
    X509          * cert = NULL;
    ASN1_TIME     * time;
    size_t          len;
    unsigned char * time_str = NULL;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(time = X509_time_f(cert)))
    {
        LOGE("alloc failed");
    }
    else if (!(bio = BIO_new(BIO_s_mem())))
    {
        LOGE("alloc failed");
    }
    else if (!ASN1_TIME_print(bio, time))
    {
        LOGE("alloc failed");
    }
    else if ((len = BIO_pending(bio)) == 0)
    {
        LOGD("nothing pending(?)");
    }
    else if (!(time_str = g_malloc0_n(len + 1, 1)))
    {
        LOGE("alloc failed");
    }
    else
    {
        BIO_read(bio, time_str, len);

        BIO_free(bio);
        X509_free(cert);

        LOGD("time_str \"%s\"", time_str);
        return time_str;
    }

    g_clear_pointer(&bio, BIO_free);
    g_clear_pointer(&cert, X509_free);
    g_clear_pointer(&time_str, g_free);

    LOGD("failed");
    return NULL;

} /* ssl_time_tostr */

unsigned char*
ssl_notbefore_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    return ssl_time_tostr(ssl, X509_get_notBefore);
} /* ssl_notbefore_tostr */

unsigned char*
ssl_notafter_tostr(evhtp_ssl_t * ssl)
{
    LOGD("(%p)", ssl);

    return ssl_time_tostr(ssl, X509_get_notAfter);
} /* ssl_notafter_tostr */

unsigned char*
ssl_sha1_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    const EVP_MD* md_alg;
    X509         * cert = NULL;
    unsigned int   n = 0;
    unsigned char  md[EVP_MAX_MD_SIZE];
    unsigned char* buf = NULL;
    size_t         nsz;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(md_alg = EVP_sha1()))
    {
        LOGD("sha1 is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!X509_digest(cert, md_alg, md, &n))
    {
        LOGE("alloc failed");
    }
    else if (!(nsz = 3 * n + 1))
    {
        LOGD("impossible to get here; just to maintain if/then/else flow");
    }
    else if (!(buf = (unsigned char *)g_malloc0_n(nsz, 1)))
    {
        LOGE("alloc failed");
    }
    else
    {
        size_t offset = 0;
        for (int i = 0; i < n; i++)
        {
            int sz = snprintf((char*)buf + offset, nsz - offset, "%02X%c", md[i], (i + 1 == n) ? 0 : ':');
            offset += sz;

            if (sz < 0 || offset >= nsz)
            {
                LOGE("failed");
                g_clear_pointer(&buf, g_free);
                break;
            }
        }

        X509_free(cert);

        LOGD("buf \"%s\"", buf);
        return buf;
    }

    g_clear_pointer(&cert, X509_free);

    LOGD("failed");
    return NULL;
} /* ssl_sha1 */

unsigned char*
ssl_serial_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    BIO           * bio = NULL;
    X509          * cert = NULL;
    size_t          len;
    unsigned char * ser_str = NULL;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(bio = BIO_new(BIO_s_mem())))
    {
        LOGE("alloc failed");
    }
    else if (i2a_ASN1_INTEGER(bio, X509_get_serialNumber(cert)) < 0)
    {
        LOGE("alloc failed");
    }
    else if ((len = BIO_pending(bio)) == 0)
    {
        LOGD("nothing pending(?)");
    }
    else if (!(ser_str = g_malloc0_n(len + 1, 1)))
    {
        LOGE("alloc failed");
    }
    else
    {
        BIO_read(bio, ser_str, len);

        X509_free(cert);
        BIO_free(bio);

        LOGD("ser_str \"%s\"", ser_str);
        return ser_str;
    }

    g_clear_pointer(&bio, BIO_free);
    g_clear_pointer(&cert, X509_free);

    LOGD("failed");
    return NULL;
}

unsigned char*
ssl_cipher_tostr(evhtp_ssl_t* ssl)
{
    LOGD("(%p)", ssl);

    const SSL_CIPHER * cipher;
    const char       * p;
    unsigned char    * cipher_str;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cipher = SSL_get_current_cipher(ssl)))
    {
        LOGD("cipher is null");
    }
    else if (!(p = SSL_CIPHER_get_name(cipher)))
    {
        LOGD("name is null");
    }
    else if (!(cipher_str = (unsigned char*)g_strdup(p)))
    {
        LOGE("alloc failed");
    }
    else
    {
        LOGD("cipher_str \"%s\"", cipher_str);
        return cipher_str;
    }

    LOGD("failed");
    return NULL;
}

static inline size_t
get_cert_len(const unsigned char* raw_cert_str, size_t raw_cert_len)
{
    LOGD("(%p, %zu)", raw_cert_str, raw_cert_len);

    size_t cert_len = raw_cert_len - 1;
    for (int i = 0; i < raw_cert_len - 1; i++)
    {
        if (raw_cert_str[i] == '\n')
        {
            /*
            * \n's will be converted to \r\n\t, so we must reserve
            * enough space for that much data.
            */
            cert_len += 2;
        }
    }
    return cert_len;
}

unsigned char*
ssl_cert_tostr(evhtp_ssl_t * ssl)
{
    LOGD("(%p)", ssl);

    X509          * cert = NULL;
    BIO           * bio = NULL;
    unsigned char * raw_cert_str = NULL;
    unsigned char * cert_str;
    size_t          raw_cert_len;
    size_t          cert_len;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(bio = BIO_new(BIO_s_mem())))
    {
        LOGE("alloc failed");
    }
    else if (!PEM_write_bio_X509(bio, cert))
    {
        LOGE("alloc failed");
    }
    else if (!(raw_cert_len = BIO_pending(bio)))
    {
        LOGD("nothing pending(?)");
    }
    else if (!(raw_cert_str = g_malloc0_n(raw_cert_len + 1, 1)))
    {
        LOGE("alloc failed");
    }
    else if (BIO_read(bio, raw_cert_str, raw_cert_len) != raw_cert_len)
    {
        LOGD("short read");
    }
    else if (!(cert_len = get_cert_len(raw_cert_str, raw_cert_len)))
    {
        LOGD("should never happen");
    }
    /* 2 extra chars, one for possible last char (if not '\n'), and one for NULL terminator */
    else if (!(cert_str = g_malloc0_n(cert_len + 2, 1)))
    {
        LOGE("alloc failed");
    }
    else
    {
        unsigned char* p = cert_str;
        int i;
        for (i = 0; i < raw_cert_len - 1; i++)
        {
            if (raw_cert_str[i] == '\n')
            {
                *p++ = '\r';
                *p++ = '\n';
                *p++ = '\t';
            }
            else
            {
                *p++ = raw_cert_str[i];
            }
        }

        /* Don't assume last character is '\n' */
        if (raw_cert_str[i] != '\n')
        {
            *p++ = raw_cert_str[i];
        }

        BIO_free(bio);
        X509_free(cert);
        g_free(raw_cert_str);

        LOGD("cert_str \"%s\"", cert_str);
        return cert_str;
    }

    g_clear_pointer(&bio, BIO_free);
    g_clear_pointer(&cert, X509_free);
    g_clear_pointer(&raw_cert_str, g_free);

    LOGD("failed");
    return NULL;
} /* ssl_cert_tostr */

unsigned char*
ssl_x509_ext_tostr(evhtp_ssl_t* ssl, const char* oid)
{
    LOGD("(%p, %p(%s))", ssl, oid, oid);

    unsigned char * ext_str = NULL;
    X509          * cert = NULL;
    ASN1_OBJECT   * oid_obj = NULL;

    const STACK_OF(X509_EXTENSION) * exts;
    int                   oid_pos;
    X509_EXTENSION* ext;
    ASN1_OCTET_STRING   * octet;
    const unsigned char * octet_data;
    long                  xlen;
    int                   xtag;
    int                   xclass;

    if (!ssl)
    {
        LOGD("ssl is null");
    }
    else if (!(cert = SSL_get_peer_certificate(ssl)))
    {
        LOGD("peer cert is null");
    }
    else if (!(oid_obj = OBJ_txt2obj(oid, 1)))
    {
        LOGE("alloc failed");
    }
    else if (!(exts = X509_get0_extensions(cert)))
    {
        LOGD("exts is null");
    }
    else if ((oid_pos = X509v3_get_ext_by_OBJ(exts, oid_obj, -1)) < 0)
    {
        LOGD("obj not found");
    }
    else if (!(ext = X509_get_ext(cert, oid_pos)))
    {
        LOGD("ext is null");
    }
    else if (!(octet = X509_EXTENSION_get_data(ext)))
    {
        LOGD("octet is null");
    }
    else if (!(octet_data = octet->data))
    {
        LOGD("octet data is null");
    }
    else if (ASN1_get_object(&octet_data, &xlen, &xtag, &xclass, octet->length))
    {
        LOGD("get object failed");
    }
    /* We're only supporting string data. Could optionally add support
     * for encoded binary data */
    else if ((xlen > 0 && xtag == 0x0C && octet->type == V_ASN1_OCTET_STRING)
        && !(ext_str = (unsigned char*)g_strndup((const gchar*)octet_data, xlen)))
    {
        LOGE("alloc failed");
    }

    g_clear_pointer(&oid_obj, ASN1_OBJECT_free);
    g_clear_pointer(&cert, X509_free);

    return ext_str;
} /* ssl_x509_ext_tostr */

static int
ssl_crl_ent_should_reload(ssl_crl_ent_t* crl_ent)
{
    LOGD("(%p)", crl_ent);

    struct stat statb;

    if (crl_ent->cfg->filename)
    {
        if (stat(crl_ent->cfg->filename, &statb) == -1)
        {
            /* file doesn't exist, we need to error out of this */
            return -1;
        }

#if __APPLE__
        if (statb.st_mtimespec.tv_sec > crl_ent->last_file_mod.tv_sec ||
            statb.st_mtimespec.tv_nsec > crl_ent->last_file_mod.tv_nsec) {
            /* file has been modified, so return yes */
            return 1;
        }
#else
        if (statb.st_mtime > crl_ent->last_file_mod)
        {
            return 1;
        }
#endif
    }

    if (crl_ent->cfg->dirname)
    {
        if (stat(crl_ent->cfg->dirname, &statb) == -1)
        {
            return -1;
        }

#if __APPLE__
        if (statb.st_mtimespec.tv_sec > crl_ent->last_dir_mod.tv_sec ||
            statb.st_mtimespec.tv_nsec > crl_ent->last_dir_mod.tv_nsec) {
            return 1;
        }
#else
        if (statb.st_mtime > crl_ent->last_file_mod)
        {
            return 1;
        }
#endif
    }

    return 0;
} /* ssl_crl_ent_should_reload */

int
ssl_crl_ent_reload(ssl_crl_ent_t * crl_ent)
{
    LOGD("(%p)", crl_ent);

    X509_LOOKUP * lookup;
    X509_STORE  * old_crl;
    struct stat   file_stat;
    int           res;

    if (!crl_ent)
    {
        LOGD("crl_ent is null");
    }
    /* make sure we have either (or both) a crl file or directory */
    else if (!crl_ent->cfg->filename && !crl_ent->cfg->dirname)
    {
        LOGD("filename and dirname are both null");
    }

    if ((res = ssl_crl_ent_should_reload(crl_ent)) <= 0)
    {
        /* we should error on a negative status here, but for right now we
         * figure the crl list does not need to be reloaded.
         */
        if (res < 0)
        {
            LOGE("SSL: CRL should reload error:%i", res);
        }
        return res;
    }

    /* if this crl_ent already has an allocated X509_STORE, we set it aside to
     * be free'd if all goes well with the new load.
     */
    old_crl = NULL;

    if (crl_ent->crl)
    {
        old_crl      = crl_ent->crl;
        crl_ent->crl = NULL;
    }

    /* initialize our own X509 storage stack. We utilize our own stack and our
     * own manual CRL verification because OpenSSL does not give us a method of
     * determining what a normal X509 is versus what a X509 CRL is. Since we
     * want to allow RProxy to dynamically read in CRL information as it comes
     * in, we must do it ourselves.
     */
    if ((crl_ent->crl = X509_STORE_new()) == NULL)
    {
        /* something bad happened, so add the old crl back and return an error */
        LOGE("SSL: CRL X509 STORE alloc failure");
        crl_ent->crl = old_crl;
        return -1;
    }

    if (crl_ent->cfg->filename != NULL)
    {
        if (stat(crl_ent->cfg->filename, &file_stat) == -1)
        {
            LOGE("SSL: CRL stat failed: %s", crl_ent->cfg->filename);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

        if (!S_ISREG(file_stat.st_mode))
        {
            LOGE("SSL: CRL not a regular file: %s", crl_ent->cfg->filename);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

        /* attempt to add the filename to our x509 store */
        if (!(lookup = X509_STORE_add_lookup(crl_ent->crl, X509_LOOKUP_file())))
        {
            LOGE("SSL: Cannot add CRL LOOKUP to store");
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

        /* copy over the value of the last time this file was modified which we
         * use to check whether we should roll this crl over or not when the
         * even timer is triggered.
         * */
#ifdef __APPLE__
        memcpy(&crl_ent->last_file_mod, &file_stat.st_mtimespec, sizeof(struct timespec));
#else
        crl_ent->last_file_mod = file_stat.st_mtime;
#endif

        if (!X509_LOOKUP_load_file(lookup, crl_ent->cfg->filename, X509_FILETYPE_PEM))
        {
            LOGE("SSL: Cannot add CRL to store: %s", crl_ent->cfg->filename);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }
        else
        {
            LOGI("SSL: CRL reloaded: %s ", crl_ent->cfg->filename);
        }
    }

    if (crl_ent->cfg->dirname != NULL)
    {
        if (stat(crl_ent->cfg->dirname, &file_stat) == -1)
        {
            LOGE("SSL: CRL stat failed: %s", crl_ent->cfg->dirname);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

        if (!S_ISDIR(file_stat.st_mode))
        {
            LOGE("SSL: CRL not a directory: %s", crl_ent->cfg->dirname);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

        if (!(lookup = X509_STORE_add_lookup(crl_ent->crl, X509_LOOKUP_hash_dir())))
        {
            LOGE("SSL: Cannot add CRL LOOKUP to store");
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }

#ifdef __APPLE__
        memcpy(&crl_ent->last_dir_mod, &file_stat.st_mtimespec, sizeof(struct timespec));
#else
        crl_ent->last_dir_mod = file_stat.st_mtime;
#endif


        if (!X509_LOOKUP_add_dir(lookup, crl_ent->cfg->dirname, X509_FILETYPE_PEM))
        {
            LOGE("SSL: Cannot add CRL directory to store: %s", crl_ent->cfg->dirname);
            X509_STORE_free(crl_ent->crl);
            crl_ent->crl = old_crl;
            return -1;
        }
        else
        {
            LOGI("SSL: CRL directory reloaded: %s ", crl_ent->cfg->dirname);
        }
    }

    if (old_crl)
    {
        X509_STORE_free(old_crl);
    }

    return 0;
} /* ssl_crl_ent_reload */

static void
ssl_reload_timercb(int sock, short which, void* arg)
{
    LOGD("(%d, %d, %p)", sock, which, arg);

    ssl_crl_ent_t* crl_ent;

    if (!(crl_ent = (ssl_crl_ent_t *)arg))
    {
        LOGD("arg is null");
        return;
    }

    /* TODO: log stuff here */
    pthread_mutex_lock(&crl_ent->lock);
    {
        ssl_crl_ent_reload(crl_ent);
        event_add(crl_ent->reload_timer_ev, &crl_ent->cfg->reload_timer);
    }
    pthread_mutex_unlock(&crl_ent->lock);
}

ssl_crl_ent_t*
ssl_crl_ent_new(evhtp_t* htp, ssl_crl_cfg_t* config)
{
    LOGD("(%p, %p)", htp, config);

    ssl_crl_ent_t* crl_ent;

    if (!htp)
    {
        LOGD("htp is null");
    }
    else if (!config)
    {
        LOGD("config is null");
    }
    else if (!(crl_ent = g_malloc0_n(sizeof(ssl_crl_ent_t), 1)))
    {
        LOGE("alloc failed");
    }
    else
    {
        crl_ent->cfg = config;
        crl_ent->htp = htp;
        crl_ent->reload_timer_ev = evtimer_new(htp->evbase, ssl_reload_timercb, crl_ent);
        pthread_mutex_init(&crl_ent->lock, NULL);

        ssl_crl_ent_reload(crl_ent);
        event_add(crl_ent->reload_timer_ev, &config->reload_timer);

        LOGD("crl_ent %p", crl_ent);
        return crl_ent;
    }

    LOGD("failed");
    return NULL;
}

/*
 * ssl_verify_crl was inspired by Apache's mod_ssl.  That code is licensed
 * under the Apache 2.0 license:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Following are comments lifted from the original mod_ssl code:
 *
 * OpenSSL provides the general mechanism to deal with CRLs but does not
 * use them automatically when verifying certificates, so we do it
 * explicitly here. We will check the CRL for the currently checked
 * certificate, if there is such a CRL in the store.
 *
 * We come through this procedure for each certificate in the certificate
 * chain, starting with the root-CA's certificate. At each step we've to
 * both verify the signature on the CRL (to make sure it's a valid CRL)
 * and it's revocation list (to make sure the current certificate isn't
 * revoked).  But because to check the signature on the CRL we need the
 * public key of the issuing CA certificate (which was already processed
 * one round before), we've a little problem. But we can both solve it and
 * at the same time optimize the processing by using the following
 * verification scheme (idea and code snippets borrowed from the GLOBUS
 * project):
 *
 * 1. We'll check the signature of a CRL in each step when we find a CRL
 *    through the _subject_ name of the current certificate. This CRL
 *    itself will be needed the first time in the next round, of course.
 *    But we do the signature processing one round before this where the
 *    public key of the CA is available.
 *
 * 2. We'll check the revocation list of a CRL in each step when
 *    we find a CRL through the _issuer_ name of the current certificate.
 *    This CRLs signature was then already verified one round before.
 *
 * This verification scheme allows a CA to revoke its own certificate as
 * well, of course.
 */


static int
ssl_verify_crl(int ok, X509_STORE_CTX* ctx, ssl_crl_ent_t* crl_ent)
{
    LOGD("(%d, %p, %p)", ok, ctx, crl_ent);

#if 0
    const size_t   kSz = 255;
    char           buf[kSz + 1];
    X509         * cert;
    X509_NAME    * subject;
    X509_NAME    * issuer;
    X509_CRL     * crl;
    X509_STORE_CTX store_ctx;
    EVP_PKEY     * public_key;
    X509_OBJECT    x509_obj = { 0 };
    int            res;
    int            timestamp_res;
#endif//

    if (!crl_ent)
    {
        LOGD("crl_ent is null");
        return ok;
    }

#if 0
    cert    = X509_STORE_CTX_get_current_cert(ctx);
    subject = X509_get_subject_name(cert);
    issuer  = X509_get_issuer_name(cert);

    /* lookup the CRL using the subject of the cert */
    X509_STORE_CTX_init(&store_ctx, crl_ent->crl, NULL, NULL);
    res     = X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, subject, &x509_obj);
    crl     = x509_obj.data.crl;

    if (res > 0 && crl) {
        public_key = X509_get_pubkey(cert);

        res        = X509_CRL_verify(crl, public_key);

        if (public_key != NULL) {
            EVP_PKEY_free(public_key);
        }

        if (res <= 0) {
            X509_OBJECT_free_contents(&x509_obj);
            LOGE("SSL: CRL validation failed:%i:%s", res, X509_NAME_oneline(subject, buf, kSz));
            return 0;
        }

        timestamp_res = X509_cmp_current_time(X509_CRL_get_nextUpdate(crl));

        if (timestamp_res == 0) {
            /* invalid nextupdate found */
            X509_OBJECT_free_contents(&x509_obj);
            LOGE("SSL: CRL nextupdate invalid:%s", X509_NAME_oneline(subject, buf, kSz));
            return 0;
        }

        if (timestamp_res < 0) {
            /* CRL is expired */
            X509_OBJECT_free_contents(&x509_obj);
            LOGE("SSL: CRL expired:%s", X509_NAME_oneline(subject, buf, kSz));
            return 0;
        }

        X509_OBJECT_free_contents(&x509_obj);
    }

    memset((void *)&x509_obj, 0, sizeof(x509_obj));

    X509_STORE_CTX_init(&store_ctx, crl_ent->crl, NULL, NULL);
    res = X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, issuer, &x509_obj);
    crl = x509_obj.data.crl;

    if ((res > 0) && crl != NULL) {
        int num_revoked;
        int i;


        num_revoked = sk_X509_REVOKED_num(X509_CRL_get_REVOKED(crl));

        for (i = 0; i < num_revoked; i++) {
            X509_REVOKED * revoked;
            ASN1_INTEGER * asn1_serial;

            revoked     = sk_X509_REVOKED_value(X509_CRL_get_REVOKED(crl), i);
            asn1_serial = revoked->serialNumber;

            if (!ASN1_INTEGER_cmp(asn1_serial, X509_get_serialNumber(cert))) {
                X509_OBJECT_free_contents(&x509_obj);
                LOGI("SSL: Certificate revoked by CRL:%s", X509_NAME_oneline(subject, buf, kSz));
                return 0;
            }
        }

        X509_OBJECT_free_contents(&x509_obj);
    }

#ifdef RPROXY_DEBUG
    if (ok) {
        LOGD("SSL: CRL ok", X509_NAME_oneline(subject, buf, kSz));
    }
#endif

#endif//0

    return ok;
} /* ssl_verify_crl */

int
ssl_x509_verifyfn(int ok, X509_STORE_CTX* store)
{
    LOGD("(%d, %p)", ok, store);

    char                 buf[256];
    X509               * err_cert;
    int                  err;
    int                  depth;
    SSL                * ssl;
    evhtp_connection_t * connection;
    evhtp_ssl_cfg_t    * ssl_cfg;
    rproxy_t           * rproxy;

    err_cert   = X509_STORE_CTX_get_current_cert(store);
    err        = X509_STORE_CTX_get_error(store);
    depth      = X509_STORE_CTX_get_error_depth(store);
    ssl        = X509_STORE_CTX_get_ex_data(store, SSL_get_ex_data_X509_STORE_CTX_idx());

    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

    connection = SSL_get_app_data(ssl);
    ssl_cfg    = connection->htp->ssl_cfg;
    rproxy     = evthr_get_aux(connection->thread);
    assert(rproxy != NULL);

    if (depth > ssl_cfg->verify_depth)
    {
        ok  = 0;
        err = X509_V_ERR_CERT_CHAIN_TOO_LONG;

        X509_STORE_CTX_set_error(store, err);
    }

    if (!ok)
    {
        LOGE("SSL: verify error:num=%d:%s:depth=%d:%s",
            err, X509_verify_cert_error_string(err), depth, buf);
    }

    /* right now the only thing using the evhtp argument is the crl_ent_t's, in
     * the future this will become more generic. So here we check to see if the
     * CRL checking is enabled, and if it is, do CRL verification.
     */
    if (connection->htp->arg)
    {
        ssl_crl_ent_t * crl_ent = (ssl_crl_ent_t *)connection->htp->arg;

        pthread_mutex_lock(&crl_ent->lock);
        {
            ok = ssl_verify_crl(ok, store, (ssl_crl_ent_t *)connection->htp->arg);
        }
        pthread_mutex_unlock(&crl_ent->lock);
    }

    return ok;
} /* ssl_x509_verifyfn */

int
ssl_x509_issuedcb(X509_STORE_CTX* ctx, X509* x, X509* issuer)
{
    LOGD("(%p, %p, %p)", ctx, x, issuer);
    return 1;
}
