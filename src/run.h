#include <mmap_file.h>

typedef struct {
  int n;
  struct {
    char *key;
    mmap_file_t *mm;
  } kvs[];
} run_args_t;
