/*
 * rp-cluster-configuration.c
 * Copyright (C) 2026 Wayne Ziebarth <ziebarthw@webscurity.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "macrologger.h"

#if (defined(rp_cluster_configuration_NOISY) || defined(ALL_NOISY)) && !defined(NO_rp_cluster_configuration_NOISY)
#   define NOISY_MSG_ LOGD
#else
#   define NOISY_MSG_(x, ...)
#endif

#include "rp-cluster-configuration.h"

#ifdef WITH_CFG_GUARDED

#include <glib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

typedef struct {
  uint32_t magic;
  uint32_t map_len;
  void*    map_base;
  /* payload follows, aligned */
} RpCfgGuardHdr;

#define RP_CFG_GUARD_MAGIC 0xC0DEC0DEu

static inline size_t rp_pagesz_(void) {
  static size_t ps;
  if (!ps) ps = (size_t)sysconf(_SC_PAGESIZE);
  return ps ? ps : 4096u;
}

RpClusterCfg*
rp_cluster_cfg_dup_guarded(const RpClusterCfg* src)
{
  g_return_val_if_fail(src != NULL, NULL);

  const size_t ps = rp_pagesz_();
  const size_t hdr_sz = (sizeof(RpCfgGuardHdr) + 15u) & ~15u;
  const size_t payload_sz = (sizeof(RpClusterCfg) + 15u) & ~15u;

  /* Map: [guard][usable...][guard] */
  size_t usable = hdr_sz + payload_sz;
  size_t usable_pages = (usable + ps - 1) / ps;
  size_t total_pages = usable_pages + 2;
  size_t total_len = total_pages * ps;

  guint8* base = mmap(NULL, total_len, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (base == MAP_FAILED) g_error("mmap failed");

  mprotect(base, ps, PROT_NONE);
  mprotect(base + (total_pages - 1) * ps, ps, PROT_NONE);

  guint8* usable_base = base + ps;

  RpCfgGuardHdr* hdr = (RpCfgGuardHdr*)usable_base;
  hdr->magic = RP_CFG_GUARD_MAGIC;
  hdr->map_len = (uint32_t)total_len;
  hdr->map_base = base;

  RpClusterCfg* p = (RpClusterCfg*)(usable_base + hdr_sz);
  *p = *src;
  return p;
}

void
rp_cluster_cfg_free_guarded(RpClusterCfg* p)
{
  LOGD("(%p)", p);

  g_return_if_fail(p != NULL);

  const size_t ps = rp_pagesz_();
  guint8* up = (guint8*)p;

  /* header is before payload */
  RpCfgGuardHdr* hdr = (RpCfgGuardHdr*)(up - ((sizeof(RpCfgGuardHdr) + 15u) & ~15u));
  if (hdr->magic != RP_CFG_GUARD_MAGIC) {
    LOGE("bad magic / wrong pointer / double free: p=%p magic=0x%08x",
            p, hdr->magic);
  }

  hdr->magic = 0xDEADF00Du; /* deterministically catch double free */
  munmap(hdr->map_base, hdr->map_len);
}
#if 0
// Keep a tiny list so we can munmap at exit if you want.
typedef struct {
  void*  base;
  size_t len;
} RpGuardMap;

static GMutex s_guard_maps_lock;
static GArray* s_guard_maps; // array of RpGuardMap

static void rp_guard_maps_init_(void) {
  static gsize inited = 0;
  if (g_once_init_enter(&inited)) {
    s_guard_maps = g_array_new(FALSE, FALSE, sizeof(RpGuardMap));
    g_mutex_init(&s_guard_maps_lock);
    g_once_init_leave(&inited, 1);
  }
}

void
rp_cluster_cfg_free_guarded(RpClusterCfg* p)
{
  LOGD("(%p)", p);

  g_return_if_fail(p != NULL);

  rp_guard_maps_init_();

  const size_t ps = rp_pagesz_();
  guint8* up = (guint8*)p;

  const size_t hdr_sz = (sizeof(RpCfgGuardHdr) + 15u) & ~15u;
  RpCfgGuardHdr* hdr = (RpCfgGuardHdr*)(up - hdr_sz);

  if (hdr->magic != RP_CFG_GUARD_MAGIC) {
    LOGE("bad magic / wrong pointer / double free: p=%p magic=0x%08x",
            p, hdr->magic);
  }

  hdr->magic = 0xDEADF00Du;

  guint8* base = (guint8*)hdr->map_base;
  size_t len = (size_t)hdr->map_len;

  // Protect *usable* pages (keep guard pages as PROT_NONE too).
  // Layout: [guard][usable...][guard]
  mprotect(base + ps, len - 2*ps, PROT_NONE);

  // Optionally remember it so you can munmap later (or leak intentionally in test builds)
  g_mutex_lock(&s_guard_maps_lock);
  RpGuardMap m = { .base = base, .len = len };
  g_array_append_val(s_guard_maps, m);
  g_mutex_unlock(&s_guard_maps_lock);
}
#endif//0


#endif//WITH_CFG_GUARDED
