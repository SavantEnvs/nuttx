#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Input: [outsize byte][spec...]. Each 5-byte spec is [flags][width][prec][length][conv]
 * and becomes one well-formed conversion; its argument is supplied with the matching
 * type, so the format string never reads a missing or mistyped vararg.
 */
static const char flagchars[] = "-+ #0";
static const char convs[] = "diuxXocsfFeEgGaApc%b";

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 6)
    {
      return -1;
    }

  size_t outsize = data[0];
  data++;
  size -= 1;

  for (size_t off = 0; off + 5 <= size; off += 5)
    {
      const uint8_t *sp = data + off;
      char fmt[48];
      size_t n = 0;
      fmt[n++] = '%';
      for (int i = 0; i < 5; i++)
        {
          if (sp[0] & (1 << i))
            {
              fmt[n++] = flagchars[i];
            }
        }

      if (sp[0] & 0x20)
        {
          n += sprintf(fmt + n, "%u", sp[1]);
        }

      if (sp[0] & 0x40)
        {
          n += sprintf(fmt + n, ".%u", sp[2]);
        }

      char conv = convs[sp[4] % (sizeof(convs) - 1)];
      static const char *const lens[] = {"", "hh", "h", "l", "ll", "j", "z", "t", "L"};
      const char *len = lens[sp[3] % 9];
      if (conv == 'c' || conv == 's' || conv == 'p' || conv == '%')
        {
          len = "";
        }
      else if (strchr("fFeEgGaA", conv) != NULL)
        {
          len = (sp[3] & 1) ? "L" : "";
        }
      else if (strcmp(len, "L") == 0)
        {
          len = "";
        }

      n += sprintf(fmt + n, "%s%c", len, conv);

      char *out = malloc(outsize ? outsize : 1);
      int r;
      uint64_t bits = 0;
      memcpy(&bits, sp, 5);
      bits = bits * 0x9e3779b97f4a7c15ULL;
      if (strchr("fFeEgGaA", conv) != NULL)
        {
          if (len[0] == 'L')
            {
              long double ld;
              memcpy(&ld, &bits, 8);
              memcpy((char *)&ld + 8, &sp[1], 2);
              r = snprintf(out, outsize, fmt, ld);
            }
          else
            {
              double d;
              memcpy(&d, &bits, 8);
              r = snprintf(out, outsize, fmt, d);
            }
        }
      else if (conv == 's')
        {
          static const char str[] = "the quick brown fox jumps over the lazy dog";
          r = snprintf(out, outsize, fmt, str);
        }
      else if (conv == 'p')
        {
          r = snprintf(out, outsize, fmt, (void *)(uintptr_t)bits);
        }
      else if (conv == '%' || conv == 'c')
        {
          r = snprintf(out, outsize, fmt, (int)bits);
        }
      else if (strcmp(len, "ll") == 0 || strcmp(len, "l") == 0 ||
               strcmp(len, "j") == 0 || strcmp(len, "z") == 0 ||
               strcmp(len, "t") == 0)
        {
          if (len[0] == 'l' && len[1] == 'l')
            {
              r = snprintf(out, outsize, fmt, (long long)bits);
            }
          else if (len[0] == 'l')
            {
              r = snprintf(out, outsize, fmt, (long)bits);
            }
          else if (len[0] == 'j')
            {
              r = snprintf(out, outsize, fmt, (intmax_t)bits);
            }
          else if (len[0] == 'z')
            {
              r = snprintf(out, outsize, fmt, (size_t)bits);
            }
          else
            {
              r = snprintf(out, outsize, fmt, (ptrdiff_t)bits);
            }
        }
      else
        {
          r = snprintf(out, outsize, fmt, (int)bits);
        }

      if (r >= 0 && outsize > 0 && strlen(out) >= outsize)
        {
          abort();
        }

      free(out);
    }

  return 0;
}
