#!/usr/bin/env awk -f

BEGIN {
  sym_prefix = "_"
  split("\
        _mma1_aligned_alloc \
        _mma1_calloc \
        _mma1_dallocx \
        _mma1_free \
        _mma1_mallctl \
        _mma1_mallctlbymib \
        _mma1_mallctlnametomib \
        _mma1_malloc \
        _mma1_malloc_conf \
        _mma1_malloc_message \
        _mma1_malloc_stats_print \
        _mma1_malloc_usable_size \
        _mma1_mallocx \
        _mma1_nallocx \
        _mma1_posix_memalign \
        _mma1_rallocx \
        _mma1_realloc \
        _mma1_sallocx \
        _mma1_sdallocx \
        _mma1_xallocx \
        _mma1_valloc \
        _pthread_create \
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
