#ifndef JSON_PARSER_H
#define JSON_PARSER_H 1

#include <stddef.h>
#include <stdint.h>

typedef struct json_args_st {
  size_t                 n_strings;
  size_t                 n_objects;
  size_t                 n_values;
  size_t                 n_kvs;
  size_t                 n_arrays;
  size_t                 n_ints;
  size_t                 n_doubles;
  size_t                 n_array_items;
} json_args_t;

typedef struct json_parser_st {
  const char *s;
  size_t siz;
  json_args_t args;
  size_t mem_i;
  size_t mem_n;
  uint8_t buf[];
} json_parser_t;

size_t json_parser(const char *s, size_t siz, json_parser_t *);
void  *json_parser_alloc(json_parser_t *, size_t n);
void   json_parser_reset(json_parser_t *);

#endif
