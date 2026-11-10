#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Input: [nspec byte][specs...] 0x00 [subject]. Each spec is 3 bytes [flags][width][conv]
 * that become one conversion; every argument points at a large aligned buffer and the
 * subject is capped, so a well-formed conversion can never overrun its destination.
 */
static const char convs[] = "diuxXocsfeEgGaAnp%[";

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 5)
    {
      return -1;
    }

  size_t nspec = 1 + data[0] % 3;
  if (1 + nspec * 3 + 1 > size)
    {
      return -1;
    }

  char fmt[128];
  size_t n = 0;
  int nargs = 0;
  for (size_t i = 0; i < nspec; i++)
    {
      const uint8_t *sp = data + 1 + i * 3;
      char conv = convs[sp[2] % (sizeof(convs) - 1)];
      fmt[n++] = '%';
      if (conv != '%' && (sp[0] & 1))
        {
          fmt[n++] = '*';
        }
      else if (conv != '%')
        {
          nargs++;
        }

      if (conv != '%' && (sp[0] & 2))
        {
          n += sprintf(fmt + n, "%u", 1 + sp[1] % 99);
        }

      if (strchr("diuxXo", conv) != NULL)
        {
          static const char *const lens[] = {"", "hh", "h", "l", "ll", "j", "z", "t"};
          n += sprintf(fmt + n, "%s", lens[(sp[0] >> 2) % 8]);
        }
      else if (strchr("feEgGaA", conv) != NULL && (sp[0] & 4))
        {
          fmt[n++] = (sp[0] & 8) ? 'L' : 'l';
        }

      fmt[n++] = conv;
      if (conv == '[')
        {
          if (sp[0] & 16)
            {
              fmt[n++] = '^';
            }

          fmt[n++] = ']';
          fmt[n++] = 'a' + sp[1] % 26;
          fmt[n++] = '-';
          fmt[n++] = 'a' + (sp[1] / 26) % 26;
          fmt[n++] = ']';
        }

      fmt[n++] = (sp[0] & 32) ? ' ' : ',';
    }

  fmt[n] = 0;

  const uint8_t *rest = data + 1 + nspec * 3;
  size_t rlen = size - 1 - nspec * 3;
  if (rlen > 200)
    {
      rlen = 200;
    }

  char *subj = malloc(rlen + 1);
  memcpy(subj, rest, rlen);
  subj[rlen] = 0;

  static union
    {
      long double align;
      char b[3][1024];
    } dst;
  memset(&dst, 0, sizeof(dst));

  sscanf(subj, fmt, dst.b[0], dst.b[1], dst.b[2]);

  free(subj);
  return 0;
}
