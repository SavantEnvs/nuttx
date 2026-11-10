#include <regex.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Input: [cflags byte][eflags byte][pattern] 0x00 [subject]. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 3)
    {
      return -1;
    }

  int cflags = data[0] & (REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB);
  int eflags = data[1] & (REG_NOTBOL | REG_NOTEOL);
  const uint8_t *rest = data + 2;
  size_t rlen = size - 2;
  const uint8_t *nul = memchr(rest, 0, rlen);
  size_t plen = nul ? (size_t)(nul - rest) : rlen;
  size_t slen = nul ? rlen - plen - 1 : 0;

  char *pat = malloc(plen + 1);
  char *sub = malloc(slen + 1);
  memcpy(pat, rest, plen);
  pat[plen] = 0;
  memcpy(sub, nul ? nul + 1 : rest, slen);
  sub[slen] = 0;

  regex_t re;
  int ret = -1;
  if (regcomp(&re, pat, cflags) == 0)
    {
      regmatch_t m[8];
      if (regexec(&re, sub, 8, m, eflags) == 0 && !(cflags & REG_NOSUB))
        {
          /* A reported match must lie inside the subject. */
          for (int i = 0; i < 8; i++)
            {
              if (m[i].rm_so >= 0 && (m[i].rm_eo < m[i].rm_so ||
                                      (size_t)m[i].rm_eo > slen))
                {
                  abort();
                }
            }
        }

      regfree(&re);
      ret = 0;
    }

  free(pat);
  free(sub);
  return ret;
}
