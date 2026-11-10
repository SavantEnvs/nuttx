#include <arpa/inet.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* Input: [selector byte][text]. IPv4/IPv6 text parsing and formatting, base64 decode+encode. */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 2)
    {
      return -1;
    }

  uint8_t sel = data[0] % 4;
  size_t len = size - 1;
  char *s = malloc(len + 1);
  memcpy(s, data + 1, len);
  s[len] = 0;

  switch (sel)
    {
      case 0:
        {
          struct in_addr a;
          if (inet_aton(s, &a))
            {
              char out[INET_ADDRSTRLEN];
              inet_ntop(AF_INET, &a, out, sizeof(out));
            }
        }
        break;
      case 1:
        {
          struct in_addr a;
          if (inet_pton(AF_INET, s, &a) == 1)
            {
              char out[INET_ADDRSTRLEN];
              inet_ntop(AF_INET, &a, out, sizeof(out));
            }
        }
        break;
      case 2:
        {
          struct in6_addr a;
          if (inet_pton(AF_INET6, s, &a) == 1)
            {
              char out[INET6_ADDRSTRLEN];
              inet_ntop(AF_INET6, &a, out, sizeof(out));
            }
        }
        break;
      default:
        {
          size_t cap = len + 4;
          uint8_t *bin = malloc(cap);
          int n = b64_pton(s, bin, cap);
          if (n >= 0)
            {
              size_t ecap = (size_t)n * 4 / 3 + 8;
              char *enc = malloc(ecap);
              if (b64_ntop(bin, n, enc, ecap) < 0)
                {
                  abort();
                }

              free(enc);
            }

          free(bin);
        }
        break;
    }

  free(s);
  return 0;
}
