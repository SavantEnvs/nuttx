#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Input: [8 bytes time_t][fmtlen byte][fmt][text]. strptime parses text by fmt;
 * strftime formats a valid struct tm (from gmtime_r) by fmt.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 10)
    {
      return -1;
    }

  int64_t t;
  memcpy(&t, data, 8);
  size_t flen = data[8];
  const uint8_t *rest = data + 9;
  size_t rlen = size - 9;
  if (flen > rlen)
    {
      flen = rlen;
    }

  size_t tlen = rlen - flen;
  char *fmt = malloc(flen + 1);
  char *txt = malloc(tlen + 1);
  memcpy(fmt, rest, flen);
  fmt[flen] = 0;
  memcpy(txt, rest + flen, tlen);
  txt[tlen] = 0;

  struct tm tm;
  memset(&tm, 0, sizeof(tm));
  char *end = strptime(txt, fmt, &tm);
  if (end != NULL && (end < txt || end > txt + tlen))
    {
      abort();
    }

  time_t tt = (time_t)t;
  struct tm g;
  if (gmtime_r(&tt, &g) != NULL)
    {
      char out[512];
      strftime(out, sizeof(out), fmt, &g);
    }

  free(fmt);
  free(txt);
  return 0;
}
