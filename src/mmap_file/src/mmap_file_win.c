#include <windows.h>
#include "mmap_file.h"

struct mmap_file_st {
  HANDLE file;
  HANDLE mapping;
  size_t len;
  const unsigned char *buf;
};

mmap_file_t *mmap_file(const char *fn, mmap_file_t *mm)
{
  mmap_file_t *mm      = NULL;
  HANDLE       file    = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  size_t       len     = GetFileSize(f, NULL);
  HANDLE       mapping = CreateFileMapping(f, NULL, PAGE_READONLY, 0, 0, NULL);
  const char  *buf     = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);

  mm = (mmap_file_t *)malloc(sizeof(struct mmap_file_st));
  mm->file    = file;
  mm->len     = len;
  mm->mapping = mapping;
  mm->buf     = (const unsigned char *)buf;
  return mm;
}

void mmap_free(mmap_t *mm) {
  CloseHandle(mm->m);
  CloseHandle(mm->f);
  free(mm);
}

const unsigned char * mmap_file_buf(mmap_file_t *mm)
{
  if (mm != NULL) {
    return mm->buf;
  }
  return NULL;
}

size_t mmap_file_len(mmap_file_t *mm)
{
  if (mm != NULL) {
    return mm->len;
  }
  return 0;
}
