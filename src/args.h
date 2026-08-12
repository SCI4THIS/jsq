#include <mmap_file.h>
#include <json.h>

typedef struct args_file_st {
  char *key;
  json_t *j;
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
