#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json.h>
#include <json_parser.h>
#include "run.h"


int yywrap(void)
{
  printf("yywrap\n");
  return 1; /* STOP parsing */
  return 0; /* CONTINUE parsing */
}

int main(int argc, char **argv)
{
  int i;
  const char *data;
  run_args_t *args = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <file> [, ... , <file>]>\n", argv[0]);
    exit(1);
  }

  args = (run_args_t *)calloc(1, sizeof(run_args_t) + argc * sizeof(args->kvs[0]));

  for (i=1; i<argc; i++) {
    mmap_file_t *mm = mmap_file(argv[i]);
    if (mm != NULL) {
      args->kvs[args->n].key = strdup(argv[i]);
      args->kvs[args->n].mm = mm;
      args->n++;
    }
  }
  //YY_BUFFER_STATE yy_scan_bytes  (const char * yybytes, int  _yybytes_len )
  //YY_BUFFER_STATE yy_scan_buffer (char *buf, yy_size_t siz)
  for (i=0; i<args->n; i++) {
    const char *key = args->kvs[i].key;
    mmap_file_t *mm = args->kvs[i].mm;
    json_t *j = NULL;
    json_parser_t *p = NULL;
    size_t psiz;
    size_t jsiz;

    psiz = json_parser(mmap_file_buf(mm), mmap_file_size(mm), NULL);
    p = (json_parser_t *)calloc(1, psiz);
    json_parser(mmap_file_buf(mm), mmap_file_size(mm), p);

    jsiz = json(p, NULL);
    j = (json_t *)calloc(1, jsiz);
    json(p, j);
    json_print(j);

    //yy_buffer_state = yy_scan_buffer(buf, siz);
    free(p);
    free(args->kvs[i].key);
    mmap_file_free(args->kvs[i].mm);
    free(j);
  }
  return 0;
}
