/*
 * Name mangling for public symbols is controlled by --with-mangling and
 * --with-jemalloc-prefix.  With default settings the je_ prefix is stripped by
 * these macro definitions.
 */
#ifndef JEMALLOC_NO_RENAME
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
