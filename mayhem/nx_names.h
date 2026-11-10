/* Force-included into every harness/selftest TU: build.sh renames each libc symbol it
 * compiles from libs/libc to nx_<name> (so it can sit beside the host libc and the
 * sanitizer runtimes); these macros make the harnesses call the renamed versions.
 */
#ifndef __MAYHEM_NX_NAMES_H
#define __MAYHEM_NX_NAMES_H

#define regcomp nx_regcomp
#define regexec nx_regexec
#define regfree nx_regfree
#define regerror nx_regerror
#define fnmatch nx_fnmatch
#define vsscanf nx_vsscanf
#define sscanf nx_sscanf
#define vsnprintf nx_vsnprintf
#define snprintf nx_snprintf
#define strtold nx_strtold
#define strtol nx_strtol
#define strtoul nx_strtoul
#define strtoll nx_strtoll
#define strtoull nx_strtoull
#define atof nx_atof
#define strptime nx_strptime
#define strftime nx_strftime
#define gmtime_r nx_gmtime_r
#define lzf_compress nx_lzf_compress
#define lzf_decompress nx_lzf_decompress
#define inet_pton nx_inet_pton
#define inet_aton nx_inet_aton
#define inet_ntop nx_inet_ntop
#define b64_ntop nx_b64_ntop
#define b64_pton nx_b64_pton

#endif
