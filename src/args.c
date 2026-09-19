#include <string.h>
#include <stdio.h>
#include "args.h"

void args_usage(int argc, char **argv)
{
  fprintf(stderr, "usage: %s -c [-o FILE] FILE [.. FILE]\n", argv[0]);
  fprintf(stderr, "compiles the files into a.jsq output file\n\n");
  fprintf(stderr, "usage: %s SCHEMA INPUT [.. INPUT]\n", argv[0]);
  fprintf(stderr, "where: SCHEMA is a.jsq FILE and INPUT are json FILE\n");
}

args_t *args_parse(int argc, char **argv)
{
  args_t *args = NULL;
  size_t i = 2;
  size_t j = 0;
  size_t siz = 0;
  size_t n = argc - 1;
  args_mode_t mode = MODE_CLASSIFY;
  if (argc < 3) {
    goto err;
  }
  if (strcmp(argv[1], "-c") == 0) {
    mode = MODE_COMPILE;
    n--;
  }
  for (; i<argc; i++) {
    if (mode == MODE_COMPILE && strcmp(argv[i], "-o") == 0 && i<(argc-1)) {
      i++;
      n -= 2;
      continue;
    }
  }

  siz = sizeof(args_t);
  siz += n * sizeof(const char *);
  siz += n * sizeof(json_schema_t *);
  siz += n * sizeof(json_t *);
  args = calloc(1, siz);
  args->mode = mode;

  i = 0;
  args->fn = (char **)&args->buf[i];
  i += n * sizeof(char *);
  args->j = (json_t **)&args->buf[i];
  i += n * sizeof(json_t *);

  args->n = n;

  if (mode == MODE_CLASSIFY) {
    args->io_fn = strdup(argv[1]);
  } else {
    args->io_fn = strdup("a.jsq");
  }

  for (j=0, i=2; i<argc; i++) {
    if (mode == MODE_COMPILE && strcmp(argv[i], "-o") == 0 && i<(argc-1)) {
      i++;
      free(args->io_fn);
      args->io_fn = strdup(argv[i]);
      continue;
    }
    args->fn[j++] = strdup(argv[i]);
  }

  return args;
err:
  args_usage(argc, argv);
  return NULL;
}

void args_free(args_t *args)
{
  size_t i;
  if (args == NULL)
    return;
  free(args->io_fn);
  if (args->jsh) {
    free(args->jsh);
  }
  for (i=0; i<args->n; i++) {
    free(args->fn[i]);
    if (args->j[i]) {
      free(args->j[i]);
    }
  }
  free(args);
}
