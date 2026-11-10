#include <fnmatch.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Input: [flags byte][pattern] 0x00 [string]. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 2)
    {
      return -1;
    }

  int flags = data[0] & (FNM_PATHNAME | FNM_PERIOD | FNM_NOESCAPE |
                         FNM_LEADING_DIR | FNM_CASEFOLD);
  const uint8_t *rest = data + 1;
  size_t rlen = size - 1;
  const uint8_t *nul = memchr(rest, 0, rlen);
  size_t plen = nul ? (size_t)(nul - rest) : rlen;
  size_t slen = nul ? rlen - plen - 1 : 0;

  char *pat = malloc(plen + 1);
  char *str = malloc(slen + 1);
  memcpy(pat, rest, plen);
  pat[plen] = 0;
  memcpy(str, nul ? nul + 1 : rest, slen);
  str[slen] = 0;

  fnmatch(pat, str, flags);

  free(pat);
  free(str);
  return 0;
}
