#ifndef ARGS_H__
#define ARGS_H__ 1

#include <mmap_file.h>
#include <json.h>
#include <json_schema.h>

typedef struct args_file_st {
  char *key;
  json_t *j;
  json_schema_t *js;
} args_file_t;

typedef struct args_st {
  int n_schemas;
  int n_inputs;
  args_file_t *schema;
  args_file_t *input;
  char buf[];
} args_t;

args_t *args_parse(int argc, char **argv);
void args_free(args_t *);

#endif
