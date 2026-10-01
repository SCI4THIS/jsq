#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "parser.h"
#include "args.h"

#define MAX_FILE_SIZE (1024 * 1024 * 8)

int yywrap(void)
{
  /* This is for stream-based parsing.  This tool is designed for
   * fixed chunk based parsing, so we will always return 1 to STOP */
  return 1; /* STOP parsing */
  return 0; /* CONTINUE parsing */
}

json_t *build_json(const char *fn)
{
  json_t *j = NULL;
  const char *key = fn;
  mmap_file_t *mm = mmap_file(key, MAX_FILE_SIZE);
  json_parser_t *p = NULL;
  size_t psiz;
  size_t jsiz;

  if (mm == NULL) {
    fprintf(stderr, "Error opening file: %s\n", fn);
    return NULL;
  }

  psiz = json_parser(mmap_file_buf(mm), mmap_file_size(mm), NULL);
  p = (json_parser_t *)calloc(1, psiz);
  json_parser(mmap_file_buf(mm), mmap_file_size(mm), p);

  jsiz = json(p, NULL);
  j = (json_t *)calloc(1, jsiz);
  json(p, j);

  free(p);
  mmap_file_free(mm);
  return j;
}

json_schema_t *build_json_schema(const char *fn)
{
  json_schema_t *js;
  json_schema_args_t args = { 0 };
  json_t *j = build_json(fn);
  if (j == NULL) { return NULL; }
  size_t siz = json_schema(j, &args, NULL);
  js = calloc(1, siz);
  json_schema(j, &args, js);
  free(j);
  return js;
}

json_t **build_json_harness(args_t *args)
{
  size_t                  i       = 0;
  size_t                  n       = args->n;
  json_parser_t         **p       = NULL;
  json_t                **j       = NULL;
  size_t                  siz     = 0;
  size_t                  p_siz   = 0;
  size_t                  j_siz   = 0;
  mmap_file_t            **mm     = calloc(n, sizeof(mmap_file_t *));
  char                   *data    = NULL;

  for (i=0; i<n; i++) {
    mm[i] = mmap_file(args->fn[i], MAX_FILE_SIZE);
    if (mm[i] == NULL) {
      fprintf(stderr, "Error opening file %s\n", args->fn[i]);
      goto err;
    }
    p_siz += json_parser(mmap_file_buf(mm[i]), mmap_file_size(mm[i]), NULL);
  }
  siz = n * sizeof(json_parser_t *) + p_siz;
  p = calloc(1, siz);
  data = (char *)&p[n];
  p_siz = 0;
  for (i=0; i<n; i++) {
    p[i] = (json_parser_t *)&data[p_siz];
    p_siz += json_parser(mmap_file_buf(mm[i]), mmap_file_size(mm[i]), p[i]);
    j_siz += json(p[i], NULL);
  }
  siz = n * sizeof(json_t *) + j_siz;
  j = calloc(1, siz);
  data = (char *)&j[n];
  j_siz = 0;
  for (i=0; i<n; i++) {
    j[i] = (json_t *)&data[j_siz];
    j_siz += json(p[i], j[i]);
  }
  for (i=0; i<n; i++) {
    mmap_file_free(mm[i]);
  }
  free(p);
err:
  free(mm);
  return j;
}

void compile(args_t *args)
{
  size_t                  i       = 0;
  size_t                  n       = args->n;
  size_t                  jsh_siz = 0;
  size_t                  len     = 0;
  json_t                **j       = NULL;
  json_schema_args_t     *js_args = NULL;
  json_schema_harness_t  *jsh     = NULL;
  char                   *buf     = NULL;
  FILE                   *f       = NULL;

  j = build_json_harness(args);
  if (j == NULL) { return; }
  js_args = calloc(n, sizeof(json_schema_args_t));
  jsh_siz = json_schema_harness(n, j, js_args, NULL);
  jsh = calloc(1, jsh_siz);
  json_schema_harness(n, j, js_args, jsh);
  len = json_schema_harness_write(jsh, NULL, 0);
  buf = malloc(len);
  json_schema_harness_write(jsh, buf, len);
  f = fopen(args->io_fn, "wb");
  if (f) {
    fwrite(buf, 1, len, f);
    fclose(f);
  } else {
    fwrite(buf, 1, len, stdout);
  }
  free(buf);
  //json_schema_harness_print(jsh);
  free(j);
  free(js_args);
  free(jsh);
}

void classify(args_t *args)
{
  size_t                 siz = 0;
  json_schema_harness_t *jsh = NULL;
  mmap_file_t           *mm  = mmap_file(args->io_fn, MAX_FILE_SIZE);
  size_t                 i   = 0;

  if (!mm) {
    perror("mmap_file()");
    return;
  }

  siz = json_schema_harness_read(mmap_file_buf(mm), mmap_file_size(mm), NULL);
  if (siz == 0) {
    fprintf(stderr, "Invalid jsq file\n");
    mmap_file_free(mm);
    return;
  }
  jsh = calloc(1, siz);
  json_schema_harness_read(mmap_file_buf(mm), mmap_file_size(mm), jsh);
  mmap_file_free(mm);

  if (args->n == 0) {
    json_schema_harness_print(jsh);
  }

  for (i=0; i<args->n; i++) {
    size_t idx;
    json_t *j = build_json(args->fn[i]);
    if (j == NULL) { continue; }
    idx = json_schema_harness_classify(jsh, j);
    printf("[%zu]: %s\n", idx, args->fn[i]);
    free(j);
  }

  free(jsh);
}

int main(int argc, char **argv)
{
  int i;
  const char *data;

  args_t *args = args_parse(argc, argv);

  if (!args) {
    exit(1);
  }

  switch (args->mode) {
    case MODE_COMPILE:
      compile(args);
      break;
    case MODE_CLASSIFY:
      classify(args);
      break;
    default:
      break;
  }

  args_free(args);
  return 0;
}
