/*
Unless otherwise specified, files in the jemalloc source distribution are
subject to the following license:
--------------------------------------------------------------------------------
Copyright (C) 2002-2017 Jason Evans <jasone@canonware.com>.
All rights reserved.
Copyright (C) 2007-2012 Mozilla Foundation.  All rights reserved.
Copyright (C) 2009-2017 Facebook, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice(s),
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice(s),
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
EVENT SHALL THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------------
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Defined if __attribute__((...)) syntax is supported. */
#define MMA1_JEMALLOC_HAVE_ATTR

/* Defined if alloc_size attribute is supported. */
#define MMA1_JEMALLOC_HAVE_ATTR_ALLOC_SIZE

/* Defined if format(gnu_printf, ...) attribute is supported. */
#define MMA1_JEMALLOC_HAVE_ATTR_FORMAT_GNU_PRINTF

/* Defined if format(printf, ...) attribute is supported. */
#define MMA1_JEMALLOC_HAVE_ATTR_FORMAT_PRINTF

/*
 * Define overrides for non-standard allocator-related functions if they are
 * present on the system.
 */
#define MMA1_JEMALLOC_OVERRIDE_MEMALIGN
#define MMA1_JEMALLOC_OVERRIDE_VALLOC

/*
 * At least Linux omits the "const" in:
 *
 *   size_t malloc_usable_size(const void *ptr);
 *
 * Match the operating system's prototype.
 */
#define MMA1_JEMALLOC_USABLE_SIZE_CONST

/*
 * If defined, specify throw() for the public function prototypes when compiling
 * with C++.  The only justification for this is to match the prototypes that
 * glibc defines.
 */
#define MMA1_JEMALLOC_USE_CXX_THROW

#ifdef _MSC_VER
#  ifdef _WIN64
#    define LG_SIZEOF_PTR_WIN 3
#  else
#    define LG_SIZEOF_PTR_WIN 2
#  endif
#endif

/* sizeof(void *) == 2^LG_SIZEOF_PTR. */
#define LG_SIZEOF_PTR 3

/*
 * Name mangling for public symbols is controlled by --with-mangling and
 * --with-jemalloc-prefix.  With default settings the je_ prefix is stripped by
 * these macro definitions.
 */
#ifndef MMA1_JEMALLOC_NO_RENAME
#  define je_aligned_alloc mma1_aligned_alloc
#  define je_calloc mma1_calloc
#  define je_dallocx mma1_dallocx
#  define je_free mma1_free
#  define je_mallctl mma1_mallctl
#  define je_mallctlbymib mma1_mallctlbymib
#  define je_mallctlnametomib mma1_mallctlnametomib
#  define je_malloc mma1_malloc
#  define je_malloc_conf mma1_malloc_conf
#  define je_malloc_message mma1_malloc_message
#  define je_malloc_stats_print mma1_malloc_stats_print
#  define je_malloc_usable_size mma1_malloc_usable_size
#  define je_mallocx mma1_mallocx
#  define je_nallocx mma1_nallocx
#  define je_posix_memalign mma1_posix_memalign
#  define je_rallocx mma1_rallocx
#  define je_realloc mma1_realloc
#  define je_sallocx mma1_sallocx
#  define je_sdallocx mma1_sdallocx
#  define je_xallocx mma1_xallocx
#  define je_memalign mma1_memalign
#  define je_valloc mma1_valloc
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <strings.h>

#define MMA1_JEMALLOC_VERSION "5.0.1-0-g896ed3a8b3f41998d4fb4d625d30ac63ef2d51fb"
#define MMA1_JEMALLOC_VERSION_MAJOR 5
#define MMA1_JEMALLOC_VERSION_MINOR 0
#define MMA1_JEMALLOC_VERSION_BUGFIX 1
#define MMA1_JEMALLOC_VERSION_NREV 0
#define MMA1_JEMALLOC_VERSION_GID "896ed3a8b3f41998d4fb4d625d30ac63ef2d51fb"

#define MALLOCX_LG_ALIGN(la)	((int)(la))
#if LG_SIZEOF_PTR == 2
#  define MALLOCX_ALIGN(a)	((int)(ffs((int)(a))-1))
#else
#  define MALLOCX_ALIGN(a)						\
     ((int)(((size_t)(a) < (size_t)INT_MAX) ? ffs((int)(a))-1 :	\
     ffs((int)(((size_t)(a))>>32))+31))
#endif
#define MALLOCX_ZERO	((int)0x40)
/*
 * Bias tcache index bits so that 0 encodes "automatic tcache management", and 1
 * encodes MALLOCX_TCACHE_NONE.
 */
#define MALLOCX_TCACHE(tc)	((int)(((tc)+2) << 8))
#define MALLOCX_TCACHE_NONE	MALLOCX_TCACHE(-1)
/*
 * Bias arena index bits so that 0 encodes "use an automatically chosen arena".
 */
#define MALLOCX_ARENA(a)	((((int)(a))+1) << 20)

/*
 * Use as arena index in "arena.<i>.{purge,decay,dss}" and
 * "stats.arenas.<i>.*" mallctl interfaces to select all arenas.  This
 * definition is intentionally specified in raw decimal format to support
 * cpp-based string concatenation, e.g.
 *
 *   #define STRINGIFY_HELPER(x) #x
 *   #define STRINGIFY(x) STRINGIFY_HELPER(x)
 *
 *   mallctl("arena." STRINGIFY(MMA1_MALLCTL_ARENAS_ALL) ".purge", NULL, NULL, NULL,
 *       0);
 */
#define MMA1_MALLCTL_ARENAS_ALL	4096
/*
 * Use as arena index in "stats.arenas.<i>.*" mallctl interfaces to select
 * destroyed arenas.
 */
#define MMA1_MALLCTL_ARENAS_DESTROYED	4097

#if defined(__cplusplus) && defined(MMA1_JEMALLOC_USE_CXX_THROW)
#  define MMA1_JEMALLOC_CXX_THROW throw()
#else
#  define MMA1_JEMALLOC_CXX_THROW
#endif

#if defined(_MSC_VER)
#  define MMA1_JEMALLOC_ATTR(s)
#  define MMA1_JEMALLOC_ALIGNED(s) __declspec(align(s))
#  define MMA1_JEMALLOC_ALLOC_SIZE(s)
#  define MMA1_JEMALLOC_ALLOC_SIZE2(s1, s2)
#  ifndef MMA1_JEMALLOC_EXPORT
#    ifdef DLLEXPORT
#      define MMA1_JEMALLOC_EXPORT __declspec(dllexport)
#    else
#      define MMA1_JEMALLOC_EXPORT __declspec(dllimport)
#    endif
#  endif
#  define MMA1_JEMALLOC_FORMAT_PRINTF(s, i)
#  define MMA1_JEMALLOC_NOINLINE __declspec(noinline)
#  ifdef __cplusplus
#    define MMA1_JEMALLOC_NOTHROW __declspec(nothrow)
#  else
#    define MMA1_JEMALLOC_NOTHROW
#  endif
#  define MMA1_JEMALLOC_SECTION(s) __declspec(allocate(s))
#  define MMA1_JEMALLOC_RESTRICT_RETURN __declspec(restrict)
#  if _MSC_VER >= 1900 && !defined(__EDG__)
#    define MMA1_JEMALLOC_ALLOCATOR __declspec(allocator)
#  else
#    define MMA1_JEMALLOC_ALLOCATOR
#  endif
#elif defined(MMA1_JEMALLOC_HAVE_ATTR)
#  define MMA1_JEMALLOC_ATTR(s) __attribute__((s))
#  define MMA1_JEMALLOC_ALIGNED(s) MMA1_JEMALLOC_ATTR(aligned(s))
#  ifdef MMA1_JEMALLOC_HAVE_ATTR_ALLOC_SIZE
#    define MMA1_JEMALLOC_ALLOC_SIZE(s) MMA1_JEMALLOC_ATTR(alloc_size(s))
#    define MMA1_JEMALLOC_ALLOC_SIZE2(s1, s2) MMA1_JEMALLOC_ATTR(alloc_size(s1, s2))
#  else
#    define MMA1_JEMALLOC_ALLOC_SIZE(s)
#    define MMA1_JEMALLOC_ALLOC_SIZE2(s1, s2)
#  endif
#  ifndef MMA1_JEMALLOC_EXPORT
#    define MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ATTR(visibility("default"))
#  endif
#  ifdef MMA1_JEMALLOC_HAVE_ATTR_FORMAT_GNU_PRINTF
#    define MMA1_JEMALLOC_FORMAT_PRINTF(s, i) MMA1_JEMALLOC_ATTR(format(gnu_printf, s, i))
#  elif defined(MMA1_JEMALLOC_HAVE_ATTR_FORMAT_PRINTF)
#    define MMA1_JEMALLOC_FORMAT_PRINTF(s, i) MMA1_JEMALLOC_ATTR(format(printf, s, i))
#  else
#    define MMA1_JEMALLOC_FORMAT_PRINTF(s, i)
#  endif
#  define MMA1_JEMALLOC_NOINLINE MMA1_JEMALLOC_ATTR(noinline)
#  define MMA1_JEMALLOC_NOTHROW MMA1_JEMALLOC_ATTR(nothrow)
#  define MMA1_JEMALLOC_SECTION(s) MMA1_JEMALLOC_ATTR(section(s))
#  define MMA1_JEMALLOC_RESTRICT_RETURN
#  define MMA1_JEMALLOC_ALLOCATOR
#else
#  define MMA1_JEMALLOC_ATTR(s)
#  define MMA1_JEMALLOC_ALIGNED(s)
#  define MMA1_JEMALLOC_ALLOC_SIZE(s)
#  define MMA1_JEMALLOC_ALLOC_SIZE2(s1, s2)
#  define MMA1_JEMALLOC_EXPORT
#  define MMA1_JEMALLOC_FORMAT_PRINTF(s, i)
#  define MMA1_JEMALLOC_NOINLINE
#  define MMA1_JEMALLOC_NOTHROW
#  define MMA1_JEMALLOC_SECTION(s)
#  define MMA1_JEMALLOC_RESTRICT_RETURN
#  define MMA1_JEMALLOC_ALLOCATOR
#endif

/*
 * The je_ prefix on the following public symbol declarations is an artifact
 * of namespace management, and should be omitted in application code unless
 * MMA1_JEMALLOC_NO_DEMANGLE is defined (see MMA1_JEMALLOC_mangle.h).
 */
extern MMA1_JEMALLOC_EXPORT const char	*je_malloc_conf;
extern MMA1_JEMALLOC_EXPORT void		(*je_malloc_message)(void *cbopaque,
    const char *s);

MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_malloc(size_t size)
    MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ATTR(malloc) MMA1_JEMALLOC_ALLOC_SIZE(1);
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_calloc(size_t num, size_t size)
    MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ATTR(malloc) MMA1_JEMALLOC_ALLOC_SIZE2(1, 2);
MMA1_JEMALLOC_EXPORT int MMA1_JEMALLOC_NOTHROW	je_posix_memalign(void **memptr,
    size_t alignment, size_t size) MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ATTR(nonnull(1));
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_aligned_alloc(size_t alignment,
    size_t size) MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ATTR(malloc)
    MMA1_JEMALLOC_ALLOC_SIZE(2);
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_realloc(void *ptr, size_t size)
    MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ALLOC_SIZE(2);
MMA1_JEMALLOC_EXPORT void MMA1_JEMALLOC_NOTHROW	je_free(void *ptr)
    MMA1_JEMALLOC_CXX_THROW;

MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_mallocx(size_t size, int flags)
    MMA1_JEMALLOC_ATTR(malloc) MMA1_JEMALLOC_ALLOC_SIZE(1);
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_rallocx(void *ptr, size_t size,
    int flags) MMA1_JEMALLOC_ALLOC_SIZE(2);
MMA1_JEMALLOC_EXPORT size_t MMA1_JEMALLOC_NOTHROW	je_xallocx(void *ptr, size_t size,
    size_t extra, int flags);
MMA1_JEMALLOC_EXPORT size_t MMA1_JEMALLOC_NOTHROW	je_sallocx(const void *ptr,
    int flags) MMA1_JEMALLOC_ATTR(pure);
MMA1_JEMALLOC_EXPORT void MMA1_JEMALLOC_NOTHROW	je_dallocx(void *ptr, int flags);
MMA1_JEMALLOC_EXPORT void MMA1_JEMALLOC_NOTHROW	je_sdallocx(void *ptr, size_t size,
    int flags);
MMA1_JEMALLOC_EXPORT size_t MMA1_JEMALLOC_NOTHROW	je_nallocx(size_t size, int flags)
    MMA1_JEMALLOC_ATTR(pure);

MMA1_JEMALLOC_EXPORT int MMA1_JEMALLOC_NOTHROW	je_mallctl(const char *name,
    void *oldp, size_t *oldlenp, void *newp, size_t newlen);
MMA1_JEMALLOC_EXPORT int MMA1_JEMALLOC_NOTHROW	je_mallctlnametomib(const char *name,
    size_t *mibp, size_t *miblenp);
MMA1_JEMALLOC_EXPORT int MMA1_JEMALLOC_NOTHROW	je_mallctlbymib(const size_t *mib,
    size_t miblen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
MMA1_JEMALLOC_EXPORT void MMA1_JEMALLOC_NOTHROW	je_malloc_stats_print(
    void (*write_cb)(void *, const char *), void *je_cbopaque,
    const char *opts);
MMA1_JEMALLOC_EXPORT size_t MMA1_JEMALLOC_NOTHROW	je_malloc_usable_size(
    MMA1_JEMALLOC_USABLE_SIZE_CONST void *ptr) MMA1_JEMALLOC_CXX_THROW;

#ifdef MMA1_JEMALLOC_OVERRIDE_MEMALIGN
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_memalign(size_t alignment, size_t size)
    MMA1_JEMALLOC_CXX_THROW MMA1_JEMALLOC_ATTR(malloc);
#endif

#ifdef MMA1_JEMALLOC_OVERRIDE_VALLOC
MMA1_JEMALLOC_EXPORT MMA1_JEMALLOC_ALLOCATOR MMA1_JEMALLOC_RESTRICT_RETURN
    void MMA1_JEMALLOC_NOTHROW	*je_valloc(size_t size) MMA1_JEMALLOC_CXX_THROW
    MMA1_JEMALLOC_ATTR(malloc);
#endif

typedef struct extent_hooks_s extent_hooks_t;

/*
 * void *
 * extent_alloc(extent_hooks_t *extent_hooks, void *new_addr, size_t size,
 *     size_t alignment, bool *zero, bool *commit, unsigned arena_ind);
 */
typedef void *(extent_alloc_t)(extent_hooks_t *, void *, size_t, size_t, bool *,
    bool *, unsigned);

/*
 * bool
 * extent_dalloc(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     bool committed, unsigned arena_ind);
 */
typedef bool (extent_dalloc_t)(extent_hooks_t *, void *, size_t, bool,
    unsigned);

/*
 * void
 * extent_destroy(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     bool committed, unsigned arena_ind);
 */
typedef void (extent_destroy_t)(extent_hooks_t *, void *, size_t, bool,
    unsigned);

/*
 * bool
 * extent_commit(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     size_t offset, size_t length, unsigned arena_ind);
 */
typedef bool (extent_commit_t)(extent_hooks_t *, void *, size_t, size_t, size_t,
    unsigned);

/*
 * bool
 * extent_decommit(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     size_t offset, size_t length, unsigned arena_ind);
 */
typedef bool (extent_decommit_t)(extent_hooks_t *, void *, size_t, size_t,
    size_t, unsigned);

/*
 * bool
 * extent_purge(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     size_t offset, size_t length, unsigned arena_ind);
 */
typedef bool (extent_purge_t)(extent_hooks_t *, void *, size_t, size_t, size_t,
    unsigned);

/*
 * bool
 * extent_split(extent_hooks_t *extent_hooks, void *addr, size_t size,
 *     size_t size_a, size_t size_b, bool committed, unsigned arena_ind);
 */
typedef bool (extent_split_t)(extent_hooks_t *, void *, size_t, size_t, size_t,
    bool, unsigned);

/*
 * bool
 * extent_merge(extent_hooks_t *extent_hooks, void *addr_a, size_t size_a,
 *     void *addr_b, size_t size_b, bool committed, unsigned arena_ind);
 */
typedef bool (extent_merge_t)(extent_hooks_t *, void *, size_t, void *, size_t,
    bool, unsigned);

struct extent_hooks_s {
        extent_alloc_t		*alloc;
        extent_dalloc_t		*dalloc;
        extent_destroy_t	*destroy;
        extent_commit_t		*commit;
        extent_decommit_t	*decommit;
        extent_purge_t		*purge_lazy;
        extent_purge_t		*purge_forced;
        extent_split_t		*split;
        extent_merge_t		*merge;
};

/*
 * By default application code must explicitly refer to mangled symbol names,
 * so that it is possible to use jemalloc in conjunction with another allocator
 * in the same application.  Define MMA1_JEMALLOC_MANGLE in order to cause automatic
 * name mangling that matches the API prefixing that happened as a result of
 * --with-mangling and/or --with-jemalloc-prefix configuration settings.
 */
#ifdef MMA1_JEMALLOC_MANGLE
#  ifndef MMA1_JEMALLOC_NO_DEMANGLE
#    define MMA1_JEMALLOC_NO_DEMANGLE
#  endif
#  define aligned_alloc je_aligned_alloc
#  define calloc je_calloc
#  define dallocx je_dallocx
#  define free je_free
#  define mallctl je_mallctl
#  define mallctlbymib je_mallctlbymib
#  define mallctlnametomib je_mallctlnametomib
#  define malloc je_malloc
#  define malloc_conf je_malloc_conf
#  define malloc_message je_malloc_message
#  define malloc_stats_print je_malloc_stats_print
#  define malloc_usable_size je_malloc_usable_size
#  define mallocx je_mallocx
#  define nallocx je_nallocx
#  define posix_memalign je_posix_memalign
#  define rallocx je_rallocx
#  define realloc je_realloc
#  define sallocx je_sallocx
#  define sdallocx je_sdallocx
#  define xallocx je_xallocx
#  define memalign je_memalign
#  define valloc je_valloc
#endif

/*
 * The je_* macros can be used as stable alternative names for the
 * public jemalloc API if MMA1_JEMALLOC_NO_DEMANGLE is defined.  This is primarily
 * meant for use in jemalloc itself, but it can be used by application code to
 * provide isolation from the name mangling specified via --with-mangling
 * and/or --with-jemalloc-prefix.
 */
#ifndef MMA1_JEMALLOC_NO_DEMANGLE
#  undef je_aligned_alloc
#  undef je_calloc
#  undef je_dallocx
#  undef je_free
#  undef je_mallctl
#  undef je_mallctlbymib
#  undef je_mallctlnametomib
#  undef je_malloc
#  undef je_malloc_conf
#  undef je_malloc_message
#  undef je_malloc_stats_print
#  undef je_malloc_usable_size
#  undef je_mallocx
#  undef je_nallocx
#  undef je_posix_memalign
#  undef je_rallocx
#  undef je_realloc
#  undef je_sallocx
#  undef je_sdallocx
#  undef je_xallocx
#  undef je_memalign
#  undef je_valloc
#endif

#ifdef __cplusplus
}
#endif
