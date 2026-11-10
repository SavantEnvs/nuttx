#include <fnmatch.h>
#include <lzf.h>
#include <netinet/in.h>
#include <regex.h>
#include <resolv.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Known-answer checks over NuttX's libc, each printing one deterministic line. */

static void kat_regex(void)
{
  regex_t re;
  regmatch_t m[3];
  if (regcomp(&re, "a(b+)c", REG_EXTENDED) != 0)
    {
      puts("regex: compile-failed");
      return;
    }

  if (regexec(&re, "xxabbbcx", 3, m, 0) != 0)
    {
      puts("regex: no-match");
    }
  else
    {
      printf("regex: %d-%d %d-%d\n", m[0].rm_so, m[0].rm_eo, m[1].rm_so, m[1].rm_eo);
    }

  regfree(&re);
}

static void kat_fnmatch(void)
{
  printf("fnmatch: %d %d %d\n", fnmatch("*.[ch]", "main.c", 0),
         fnmatch("*.[ch]", "main.o", 0), fnmatch("A?c", "abc", FNM_CASEFOLD));
}

static void kat_printf(void)
{
  char b[64];
  snprintf(b, sizeof(b), "[%5d|%-5d|%05d|%x|%#o|%s|%.3f]", 42, 42, 42, 255, 8, "hi", 3.14159);
  printf("printf: %s\n", b);
}

static void kat_scanf(void)
{
  int a = 0;
  unsigned x = 0;
  char s[16] = "";
  int n = sscanf("12 ff word", "%d %x %15s", &a, &x, s);
  printf("scanf: %d %d %u %s\n", n, a, x, s);
}

static void kat_strtox(void)
{
  char *e;
  long a = strtol("  -1234xyz", &e, 10);
  unsigned long b = strtoul("0xff", NULL, 0);
  long long c = strtoll("-7fffffffffffffff", NULL, 16);
  long double d = strtold("2.5e2", NULL);
  printf("strtox: %ld %s %lu %lld %d\n", a, e, b, c, (int)d);
}

static void kat_time(void)
{
  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  char *end = strptime("2024-03-05 07:08:09", "%Y-%m-%d %H:%M:%S", &tm);
  time_t t = 1700000000;
  struct tm g;
  char out[64];
  gmtime_r(&t, &g);
  strftime(out, sizeof(out), "%Y-%m-%d %H:%M:%S", &g);
  printf("time: %d %d-%d-%d %s\n", end != NULL && *end == 0, tm.tm_year, tm.tm_mon, tm.tm_mday, out);
}

static void kat_lzf(void)
{
  uint8_t ib[8 + 256], ob[8 + 512], back[256];
  for (int i = 0; i < 256; i++)
    {
      ib[8 + i] = "abcabcabd"[i % 9];
    }

  static lzf_state_t htab;
  struct lzf_header_s *hdr = NULL;
  size_t n = lzf_compress(ib + 8, 256, ob + 8, 512, htab, &hdr);
  unsigned got = 0;
  if (n > LZF_TYPE1_HDR_SIZE)
    {
      got = lzf_decompress((uint8_t *)hdr + LZF_TYPE1_HDR_SIZE, n - LZF_TYPE1_HDR_SIZE, back, sizeof(back));
    }

  printf("lzf: %d %u %d\n", n > 0 && n < 100, got, got == 256 && memcmp(back, ib + 8, 256) == 0);
}

static void kat_inet(void)
{
  struct in_addr a4;
  struct in6_addr a6;
  char o4[INET_ADDRSTRLEN], o6[INET6_ADDRSTRLEN];
  inet_pton(AF_INET, "192.168.1.20", &a4);
  inet_pton(AF_INET6, "2001:db8::ff00:42:8329", &a6);
  inet_ntop(AF_INET, &a4, o4, sizeof(o4));
  inet_ntop(AF_INET6, &a6, o6, sizeof(o6));
  unsigned char bin[32];
  char enc[64];
  int dn = b64_pton("aGVsbG8gd29ybGQ=", bin, sizeof(bin));
  bin[dn > 0 ? dn : 0] = 0;
  b64_ntop((const unsigned char *)"nuttx", 5, enc, sizeof(enc));
  printf("inet: %s %s %d %s %s\n", o4, o6, dn, bin, enc);
}

int main(void)
{
  kat_regex();
  kat_fnmatch();
  kat_printf();
  kat_scanf();
  kat_strtox();
  kat_time();
  kat_lzf();
  kat_inet();
  return 0;
}
