#ifndef ARGS_H__
#define ARGS_H__ 1

#include <mmap_file.h>
#include <json.h>
#include <json_schema.h>

typedef enum {
  MODE_COMPILE = 1,
  MODE_CLASSIFY = 2,
} args_mode_t;

typedef struct args_st {
  args_mode_t mode;
  int n;
  char *io_fn;
  json_schema_harness_t *jsh;
  char **fn;
  json_t **j;
  char buf[];
} args_t;

args_t *args_parse(int argc, char **argv);
void args_free(args_t *);

#endif
