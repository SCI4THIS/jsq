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


int main(int argc, char **argv)
{
  int i;
  const char *data;

  args_t *args = args_parse(argc, argv);

  if (!args) {
    exit(1);
  }

  //YY_BUFFER_STATE yy_scan_bytes  (const char * yybytes, int  _yybytes_len )
  //YY_BUFFER_STATE yy_scan_buffer (char *buf, yy_size_t siz)
  for (i=0; i<args->n_schemas; i++) {
    args->js[i] = build_json_schema(args->js_fn[i]);
  }
  for (i=0; i<args->n_inputs; i++) {
    args->j[i] = build_json(args->j_fn[i]);
  }
  /*
  for (i=0; i<args->n_schemas; i++) {
    json_schema_print(args->js[i]);
  }
  */
  for (i=0; i<args->n_inputs; i++) {
    /* TODO: fix for routing */
    bool is_valid = json_schema_validate(args->js[0], args->j[i]);
    printf("[%zu]: %s\n", i, is_valid ? "VALID" : "INVALID");
  }
  args_free(args);
  return 0;
}
