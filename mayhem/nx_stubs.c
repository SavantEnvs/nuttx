/* The two kernel services the portable libc code calls: errno storage and assert. */
#include <stdio.h>
#include <stdlib.h>

static int g_errno;

int *nx___errno(void)
{
  return &g_errno;
}

void nx___assert(const char *filename, int linenum, const char *msg)
{
  fprintf(stderr, "NuttX assert: %s:%d: %s\n", filename, linenum, msg);
  abort();
}
