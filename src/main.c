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
    args->schema[i].j = build_json(args->schema[i].key);
  }
  for (i=0; i<args->n_inputs; i++) {
    args->input[i].j = build_json(args->input[i].key);
  }
  for (i=0; i<args->n_schemas; i++) {
    json_print(args->schema[i].j);
  }
  for (i=0; i<args->n_inputs; i++) {
    json_print(args->input[i].j);
  }
  args_free(args);
  return 0;
}
