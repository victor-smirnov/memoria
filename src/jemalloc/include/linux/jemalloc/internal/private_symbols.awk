#!/usr/bin/env awk -f

BEGIN {
  sym_prefix = ""
  split("\
        mma1_aligned_alloc \
        mma1_calloc \
        mma1_dallocx \
        mma1_free \
        mma1_mallctl \
        mma1_mallctlbymib \
        mma1_mallctlnametomib \
        mma1_malloc \
        mma1_malloc_conf \
        mma1_malloc_message \
        mma1_malloc_stats_print \
        mma1_malloc_usable_size \
        mma1_mallocx \
        mma1_nallocx \
        mma1_posix_memalign \
        mma1_rallocx \
        mma1_realloc \
        mma1_sallocx \
        mma1_sdallocx \
        mma1_xallocx \
        mma1_memalign \
        mma1_valloc \
        pthread_create \
        ", exported_symbol_names)
  # Store exported symbol names as keys in exported_symbols.
  for (i in exported_symbol_names) {
    exported_symbols[exported_symbol_names[i]] = 1
  }
}

# Process 'nm -a <c_source.o>' output.
#
# Handle lines like:
#   0000000000000008 D opt_junk
#   0000000000007574 T malloc_initialized
(NF == 3 && $2 ~ /^[ABCDGRSTVW]$/ && !($3 in exported_symbols) && $3 ~ /^[A-Za-z0-9_]+$/) {
  print substr($3, 1+length(sym_prefix), length($3)-length(sym_prefix))
}

# Process 'dumpbin /SYMBOLS <c_source.obj>' output.
#
# Handle lines like:
#   353 00008098 SECT4  notype       External     | opt_junk
#   3F1 00000000 SECT7  notype ()    External     | malloc_initialized
($3 ~ /^SECT[0-9]+/ && $(NF-2) == "External" && !($NF in exported_symbols)) {
  print $NF
}
