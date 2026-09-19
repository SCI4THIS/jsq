#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json_parser.h>
#include "args.h"


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
  mmap_file_t *mm = mmap_file(key);
  json_parser_t *p = NULL;
  size_t psiz;
  size_t jsiz;

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
  size_t                  p_siz   = 0;
  size_t                  j_siz   = 0;
  mmap_file_t            **mm     = calloc(n, sizeof(mmap_file_t *));
  char                   *data    = NULL;

  for (i=0; i<n; i++) {
    mm[i] = mmap_file(args->fn[i]);
    p_siz += json_parser(mmap_file_buf(mm[i]), mmap_file_size(mm[i]), NULL);
  }
  p = calloc(1, n * sizeof(json_parser_t *) + p_siz);
  data = (char *)&p[n];
  p_siz = 0;
  for (i=0; i<n; i++) {
    p[i] = (json_parser_t *)&data[p_siz];
    p_siz += json_parser(mmap_file_buf(mm[i]), mmap_file_size(mm[i]), p[i]);
    j_siz += json(p[i], NULL);
  }
  j = calloc(1, n * sizeof(json_t *) + j_siz);
  data = (char *)&j[n];
  j_siz = 0;
  for (i=0; i<n; i++) {
    j[i] = (json_t *)&data[j_siz];
    j_siz += json(p[i], j[i]);
  }
  for (i=0; i<n; i++) {
    mmap_file_free(mm[i]);
  }
  free(mm);
  free(p);
  return j;
}

void compile(args_t *args)
{
  size_t                  i       = 0;
  size_t                  n       = args->n;
  size_t                  jsh_siz = 0;
  json_t                **j       = NULL;
  json_schema_args_t     *js_args = NULL;
  json_schema_harness_t  *jsh     = NULL;

  j = build_json_harness(args);
  js_args = calloc(n, sizeof(json_schema_args_t));
  jsh_siz = json_schema_harness(n, j, js_args, NULL);
  jsh = calloc(1, jsh_siz);
  json_schema_harness(n, j, js_args, jsh);
  if (n == 1) {
   json_schema_print(json_schema_harness_schema(jsh, 0));
  }
  free(j);
  free(js_args);
  free(jsh);
}

void classify(args_t *args)
{
  /*
  for (i=0; i<args->n_inputs; i++) {
    bool is_valid = json_schema_validate(args->js[0], args->j[i]);
    printf("[%zu]: %s\n", i, is_valid ? "VALID" : "INVALID");
  }
  */
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
