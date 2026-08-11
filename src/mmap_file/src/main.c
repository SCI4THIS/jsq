#include <stdio.h>
#include "mmap_file.h"

void hexdump(mmap_file_t *mm)
{
  size_t      i   = 0;
  size_t      n   = mmap_file_size(mm);
  const char *buf = mmap_file_buf(mm);

  for (i=0; i<n; i++) {
    if (i > 0 && (i & 0xF) == 0) {
      printf("\n");
    }
    printf("%02X ", buf[i]);
  }
  printf("\n");
}

int main(int argc, char **argv)
{
  mmap_file_t *mm = NULL;
  if (argc != 2) {
    fprintf(stderr, "usage: %s <file>\n", argv[0]);
    return 0;
  }
  mm = mmap_file(argv[1]);
  if (mm == NULL) {
    goto err;
  }
  hexdump(mm);
  mmap_file_free(mm);
  return 0;
err:
  return 1;
}
