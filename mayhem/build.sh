#!/usr/bin/env bash
#
# mayhem/build.sh — build the NuttX libc fuzz harnesses and the KAT oracle.
#
# NuttX is an RTOS whose kernel cannot run on a fuzzing host, but its libc
# (libs/libc) is portable C. We compile the real upstream libc sources
# (printf/scanf, regex, strptime/strftime, fnmatch, lzf, inet/base64, strto*)
# against a hand-written nuttx/config.h (mayhem/nx_shim), then rename every
# symbol they define to nx_<name> so they link beside the host libc and
# the sanitizer runtimes without interposing on them.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${CXX:=clang++}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"
M="$SRC/mayhem"
B="$M/build"
RES="$("$CC" -print-resource-dir)/include"

# Upstream libc sources (relative to libs/libc) that make up the fuzzed library.
LIBC_SRCS="
stdio/lib_vsscanf.c stdio/lib_sscanf.c stdio/lib_vsprintf.c stdio/lib_vsnprintf.c
stdio/lib_snprintf.c stdio/lib_sprintf.c
stream/lib_libvsprintf.c stream/lib_libvscanf.c stream/lib_libnoflush.c stream/lib_meminstream.c
stream/lib_memoutstream.c stream/lib_nulloutstream.c stream/lib_ultoa_invert.c
stream/lib_dtoa_bits.c stream/lib_dtoa_big.c stream/lib_hexfloat.c
stdlib/lib_strtol.c stdlib/lib_strtoul.c stdlib/lib_strtoll.c stdlib/lib_strtoull.c
stdlib/lib_strtold.c stdlib/lib_atof.c stdlib/lib_checkbase.c stdlib/lib_mbtowc.c
stdlib/lib_qsort.c
wchar/lib_mbrtowc.c
time/lib_strptime.c time/lib_strftime.c time/lib_gmtime.c time/lib_gmtimer.c
time/lib_isleapyear.c time/lib_daysbeforemonth.c time/lib_dayofweek.c
regex/regcomp.c regex/regexec.c regex/tre-mem.c regex/regerror.c
lzf/lzf_d.c lzf/lzf_c.c
net/lib_inetpton.c net/lib_inetaton.c net/lib_inetntop.c net/lib_base64.c
misc/lib_fnmatch.c
"
for f in "$SRC"/libs/libc/ctype/*.c "$SRC"/libs/libc/wctype/*.c; do
  LIBC_SRCS="$LIBC_SRCS ${f#"$SRC"/libs/libc/}"
done
for f in "$SRC"/libs/libc/string/lib_*.c; do
  case "$f" in */lib_bsd*|*/lib_vikmemcpy.c|*/lib_strerror*.c|*/lib_strsignal.c|*/lib_strdup.c|*/lib_strndup.c) continue ;; esac
  LIBC_SRCS="$LIBC_SRCS ${f#"$SRC"/libs/libc/}"
done

# Flags shared by every NuttX translation unit: NuttX's own headers, no host libc headers.
NX_INC=(-ffreestanding -fno-builtin -nostdinc -isystem "$SRC/include" -isystem "$RES"
        -idirafter "$SRC/include/nuttx/lib" -I "$M/nx_shim"
        -I "$SRC/libs/libc" -I "$SRC/libs/libc/regex" -I "$SRC/libs/libc/stream"
        -Wno-everything)

# build_lib <outdir> <flags...>: compile + rename every libc source into <outdir>/*.o
build_lib() {
  local out="$1"; shift
  rm -rf "$out"; mkdir -p "$out"
  local f obj n=0 p
  local -a pids=()
  for f in $LIBC_SRCS; do
    obj="$out/$(echo "$f" | tr / _ | sed 's/\.c$/.o/')"
    # lzf_c.c adds a zero bias to the NULL hash slots on every call: benign, and it would
    # halt every compress run, so only pointer-overflow is relaxed, only for that file.
    local extra=()
    [ "$f" = lzf/lzf_c.c ] && extra=(-fno-sanitize=pointer-overflow)
    "$CC" -c "$@" "${extra[@]}" "${NX_INC[@]}" "$SRC/libs/libc/$f" -o "$obj" &
    pids+=($!)
    if (( ++n % MAYHEM_JOBS == 0 )); then for p in "${pids[@]}"; do wait "$p"; done; pids=(); fi
  done
  for p in "${pids[@]}"; do wait "$p"; done
  # Rename every defined global (plus the two stubbed externals) to nx_<name>, in
  # definitions and in references, so nothing here collides with the host libc/ASan.
  { nm -g --defined-only "$out"/*.o | awk 'NF==3{print $3}'; echo __errno; echo __assert; } \
      | sort -u | awk '{print $1" nx_"$1}' > "$out/rename.txt"
  local o
  for o in "$out"/*.o; do objcopy --redefine-syms="$out/rename.txt" "$o"; done
}

# Fuzz flavour: sanitized, coverage-instrumented, DWARF 3.
FUZZ_FLAGS=(-O1 $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link)
build_lib "$B/fuzz" "${FUZZ_FLAGS[@]}"

# Stubs the libc expects from the kernel (errno, assert), compiled against host headers.
"$CC" -c "${FUZZ_FLAGS[@]}" "$M/nx_stubs.c" -o "$B/fuzz/nx_stubs.o"
# LeakSanitizer off, at build time.
"$CXX" -c $SANITIZER_FLAGS $DEBUG_FLAGS "$M/lsan_off.cc" -o "$B/lsan_off.o"
"$CC" -c $SANITIZER_FLAGS $DEBUG_FLAGS -x c "$STANDALONE_FUZZ_MAIN" -o "$B/standalone_main.o"

: "${TARGETS:=fuzz_regex fuzz_fnmatch fuzz_scanf fuzz_printf fuzz_strtox fuzz_strptime fuzz_lzf fuzz_inet}"
for t in $TARGETS; do
  "$CC" -c "${FUZZ_FLAGS[@]}" "${NX_INC[@]}" -include "$M/nx_names.h" "$M/$t.c" -o "$B/$t.o"
  "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$B/$t.o" "$B"/fuzz/*.o "$B/lsan_off.o" -o "$SRC/$t"
  "$CC" $SANITIZER_FLAGS $DEBUG_FLAGS "$B/standalone_main.o" "$B/$t.o" "$B"/fuzz/*.o "$B/lsan_off.o" -o "$SRC/$t-standalone"
done

# Oracle flavour: NORMAL flags (no sanitizer, no coverage instrumentation), used only by test.sh.
build_lib "$B/oracle" -O2 $COVERAGE_FLAGS
"$CC" -c -O2 "$M/nx_stubs.c" -o "$B/oracle/nx_stubs.o"
"$CC" -c -O2 $COVERAGE_FLAGS "${NX_INC[@]}" -include "$M/nx_names.h" "$M/selftest.c" -o "$B/selftest.o"
"$CC" -O2 $COVERAGE_FLAGS "$B/selftest.o" "$B"/oracle/*.o -o "$SRC/nuttx_selftest"
