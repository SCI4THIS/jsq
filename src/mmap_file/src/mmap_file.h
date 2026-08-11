#include <stdlib.h>

typedef struct mmap_file_st mmap_file_t;

mmap_file_t          *mmap_file(const char *fn);
void                  mmap_file_free(mmap_file_t *);
const unsigned char  *mmap_file_buf(mmap_file_t *);
size_t                mmap_file_size(mmap_file_t *);
