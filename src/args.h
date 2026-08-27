#ifndef ARGS_H__
#define ARGS_H__ 1

#include <mmap_file.h>
#include <json.h>
#include <json_schema.h>

typedef struct args_st {
  int n_schemas;
  int n_inputs;
  char **js_fn;
  char **j_fn;
  json_schema_t **js;
  json_t **j;
  char buf[];
} args_t;

args_t *args_parse(int argc, char **argv);
void args_free(args_t *);

#endif
