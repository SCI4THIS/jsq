#include <stdint.h>
#include <json_parser.h>

typedef enum {
  JSON_VALUE_TYPE_INVALID = 0,
  JSON_VALUE_TYPE_INT = 1,
  JSON_VALUE_TYPE_DOUBLE = 2,
  JSON_VALUE_TYPE_NUM = 3, /* 1 | 2 */
  JSON_VALUE_TYPE_TRUE = 4,
  JSON_VALUE_TYPE_FALSE = 5,
  JSON_VALUE_TYPE_STRING = 6,
  JSON_VALUE_TYPE_NULL = 7,
  JSON_VALUE_TYPE_OBJECT = 8,
  JSON_VALUE_TYPE_ARRAY = 9,
} json_value_type_t;

typedef struct json_object_st json_object_t;

typedef struct json_string_st {
  const char *s;
  size_t len;
} json_string_t;

typedef struct json_num_integer_st {
  int n;
} json_int_t;

typedef struct json_num_double_st {
  double n;
} json_double_t;

typedef struct json_value_st {
  json_value_type_t type;
  void *payload;
} json_value_t;

typedef struct json_array_st json_array_t;

typedef struct json_array_item_st {
  json_value_t *value;
  struct json_array_item_st *next;
  struct json_array_item_st *prev;
  json_array_t *parent;
} json_array_item_t;

struct json_array_st {
  size_t n;
  json_array_item_t *first_item;
};


typedef struct json_kv_st {
  json_string_t *key;
  json_value_t *value;
  struct json_kv_st *next;
  struct json_kv_st *prev;
  json_object_t *parent;
} json_kv_t;

struct json_object_st {
  size_t parent;
  size_t n;
  json_kv_t *first_kv;
};

typedef struct json_st {
  json_args_t           args;
  json_string_t         *strings;
  json_object_t         *objects;
  json_value_t          *values;
  json_kv_t             *kvs;
  json_array_t          *arrays;
  json_array_item_t     *array_items;
  json_int_t            *ints;
  json_double_t         *doubles;
  char                  *stab;
  uint8_t buf[];
} json_t;

void   json_args_print(json_args_t *args);
size_t json(json_parser_t *p, json_t *j);
void   json_print(json_t *j);
void   json_defrag(json_t *j);
