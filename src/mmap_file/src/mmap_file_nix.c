#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mmap_file.h"

struct mmap_file_st {
  int                  fd;
  size_t               len;
  const unsigned char *buf;
};

mmap_file_t *mmap_file(const char *fn)
{
  mmap_file_t *mm   = NULL;
  int          fd   = 0;
  struct stat  sb   = { 0 };
  const char  *data = NULL;

  fd = open(fn, O_RDONLY);
  if (fd == -1) {
    goto err;
  }

  if (fstat(fd, &sb) == -1) {
    goto err2;
  }

  data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data == MAP_FAILED) {
    goto err2;
  }

  mm = (mmap_file_t *)malloc(sizeof(struct mmap_file_st));

  mm->fd  = fd;
  mm->len = sb.st_size;
  mm->buf = data;

  return mm;

err2:
  close(fd);
err:
  return NULL;
}

void mmap_file_free(mmap_file_t *mm) {
  if (mm == NULL) {
    return;
  }
  unsigned char *data = (unsigned char *)mm->buf;
  munmap(data, mm->len);
  close(mm->fd);
  memset(mm, 0, sizeof(*mm));
  free(mm);
}

const unsigned char * mmap_file_buf(mmap_file_t *mm)
{
  if (mm != NULL) {
    return mm->buf;
  }
  return NULL;
}

size_t mmap_file_size(mmap_file_t *mm)
{
  if (mm != NULL) {
    return mm->len;
  }
  return 0;
}
