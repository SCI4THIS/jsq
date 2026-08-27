#include <string.h>
#include <stdio.h>
#include "args.h"

void args_usage(int argc, char **argv)
{
  fprintf(stderr, "usage: %s SCHEMA(s) -i INPUT(s)\n", argv[0]);
  fprintf(stderr, "where: SCHEMA(s) and INPUT(s) are FILE [, ...., FILE]\n");
}

args_t *args_parse(int argc, char **argv)
{
  args_t *args = NULL;
  size_t i;
  size_t n_schemas = 0;
  size_t n_inputs = 0;
  size_t siz = 0;
  int mode = 0;
  if (argc < 4) {
    goto err;
  }
  for (i=1; i<argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      mode = 1;
      continue;
    }
    switch (mode) {
      case 0:
        n_schemas++;
	break;
      case 1:
	n_inputs++;
	break;
    }
  }
  if (mode != 1) {
    goto err;
  }
  siz = sizeof(args_t);
  siz += n_schemas * sizeof(const char *);
  siz += n_schemas * sizeof(json_schema_t *);
  siz += n_inputs * sizeof(const char *);
  siz += n_inputs * sizeof(json_t *);
  args = calloc(1, siz);

  i = 0;
  args->js_fn = (char **)&args->buf[i];
  i += n_schemas * sizeof(char *);
  args->js = (json_schema_t **)&args->buf[i];
  i += n_schemas * sizeof(json_schema_t *);
  args->j_fn = (char **)&args->buf[i];
  i += n_inputs * sizeof(char *);
  args->j = (json_t **)&args->buf[i];
  i += n_inputs * sizeof(json_t *);

  mode = 0;
  for (i=1; i<argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      mode = 1;
      continue;
    }
    switch (mode) {
      case 0:
	args->js_fn[args->n_schemas] = strdup(argv[i]);
        args->n_schemas++;
	break;
      case 1:
	args->j_fn[args->n_inputs] = strdup(argv[i]);
	args->n_inputs++;
	break;
    }
  }

err:
  if (args == NULL) {
    args_usage(argc, argv);
  }
  return args;
}

void args_free(args_t *args)
{
  size_t i;
  if (args == NULL)
    return;
  for (i=0; i<args->n_inputs; i++) {
    free(args->j_fn[i]);
    free(args->j[i]);
  }
  for (i=0; i<args->n_schemas; i++) {
    free(args->js_fn[i]);
    free(args->js[i]);
  }
  free(args);
}
