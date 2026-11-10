#include <lzf.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Input: [mode byte][data]. Odd mode: decompress hostile data into a buffer of
 * (mode>>1)*32 bytes. Even mode: compress data, decompress it back, require equality.
 *
 * Every input buffer has one trailing zero byte. lzf_compress reads in[1] of a 1-byte
 * input (lzf_c.c:166) and lzf_decompress reads in[0] of an empty one (lzf_d.c:74)
 * before any length check; the in-tree caller, lib_lzfoutstream, passes a slice of a
 * fixed block array where that byte is in bounds. Mayhem's startup probe (-runs=5 from
 * an empty corpus) trips both within 5 execs and fails every run. Overreads of 2+ bytes
 * still trap.
 */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 1)
    {
      return -1;
    }

  uint8_t mode = data[0];
  const uint8_t *in = data + 1;
  unsigned in_len = size - 1;

  if (mode & 1)
    {
      unsigned out_len = (unsigned)(mode >> 1) * 32 + 1;
      uint8_t *out = malloc(out_len);
      uint8_t *src = calloc(1, in_len + 1);
      memcpy(src, in, in_len);
      lzf_decompress(src, in_len, out, out_len);
      free(src);
      free(out);
      return 0;
    }

  /* lzf_compress writes a header just below both buffers. */
  uint8_t *ib = calloc(1, in_len + 8 + 1);
  memcpy(ib + 8, in, in_len);
  unsigned cap = in_len + in_len / 25 + 64;
  uint8_t *ob = malloc(cap + 8);
  lzf_state_t *htab = calloc(1, sizeof(*htab));
  struct lzf_header_s *hdr = NULL;

  size_t n = lzf_compress(ib + 8, in_len, ob + 8, cap, *htab, &hdr);
  if (n != 0 && hdr != NULL)
    {
      uint8_t *back = malloc(in_len + 1);
      const uint8_t *comp = (const uint8_t *)hdr;
      unsigned clen = n;
      unsigned hsz = (comp[2] == LZF_TYPE0_HDR) ? LZF_TYPE0_HDR_SIZE : LZF_TYPE1_HDR_SIZE;
      if (clen > hsz)
        {
          unsigned got = (comp[2] == LZF_TYPE0_HDR)
                         ? (memcpy(back, comp + hsz, clen - hsz), clen - hsz)
                         : lzf_decompress(comp + hsz, clen - hsz, back, in_len + 1);
          if (got != in_len || memcmp(back, in, in_len) != 0)
            {
              abort();
            }
        }

      free(back);
    }

  free(htab);
  free(ob);
  free(ib);
  return 0;
}
