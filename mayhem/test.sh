#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Known-answer oracle over NuttX's portable libc (regex, fnmatch, printf, scanf, strto*, time,
# lzf, inet/base64). nuttx_selftest is built by build.sh with normal flags; each stdout line is
# compared here, in bash, against the expected answer for that group.
BIN="$SRC/nuttx_selftest"
[ -x "$BIN" ] || { echo "missing $BIN: mayhem/build.sh did not build it" >&2; exit 2; }

OUT="$("$BIN" 2>&1)"

declare -A WANT=(
  [regex]='regex: 2-7 3-6'
  [fnmatch]='fnmatch: 0 1 0'
  [printf]='printf: [   42|42   |00042|ff|010|hi|3.142]'
  [scanf]='scanf: 3 12 255 word'
  [strtox]='strtox: -1234 xyz 255 -9223372036854775807 250'
  [time]='time: 1 124-2-5 2023-11-14 22:13:20'
  [lzf]='lzf: 1 256 1'
  [inet]='inet: 192.168.1.20 2001:db8::ff00:42:8329 11 hello world bnV0dHg='
)

passed=0; failed=0
for k in regex fnmatch printf scanf strtox time lzf inet; do
  got="$(printf '%s\n' "$OUT" | grep -m1 "^$k: ")"
  if [ "$got" = "${WANT[$k]}" ]; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL $k: want '${WANT[$k]}' got '$got'" >&2
  fi
done

emit_ctrf nuttx-libc-kat "$passed" "$failed"
