#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Input: [selector byte][base byte][text]. Checks endptr stays inside the text. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 3)
    {
      return -1;
    }

  unsigned sel = data[0] % 6;
  int base = data[1] % 40;
  size_t len = size - 2;
  char *s = malloc(len + 1);
  memcpy(s, data + 2, len);
  s[len] = 0;

  char *end = NULL;
  switch (sel)
    {
      case 0: strtol(s, &end, base); break;
      case 1: strtoul(s, &end, base); break;
      case 2: strtoll(s, &end, base); break;
      case 3: strtoull(s, &end, base); break;
      case 4: strtold(s, &end); break;
      default: atof(s); end = s; break;
    }

  if (end < s || end > s + len)
    {
      abort();
    }

  free(s);
  return 0;
}
