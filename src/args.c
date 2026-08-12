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
  args = calloc(1, sizeof(args_t) +
                   (n_schemas + n_inputs) * sizeof(args_file_t));
  args->schema = (args_file_t *)&args->buf[0];
  args->input = (args_file_t *)&args->buf[n_schemas * sizeof(args_file_t)];
  mode = 0;
  for (i=1; i<argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      mode = 1;
      continue;
    }
    switch (mode) {
      case 0:
	args->schema[args->n_schemas].key = strdup(argv[i]);
        args->n_schemas++;
	break;
      case 1:
	args->input[args->n_inputs].key = strdup(argv[i]);
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
    free(args->input[i].key);
    free(args->input[i].j);
  }
  for (i=0; i<args->n_schemas; i++) {
    free(args->schema[i].key);
    free(args->schema[i].j);
  }
  free(args);
}
