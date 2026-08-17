#ifndef JSON_INT_H__
#define JSON_INT_H__ 1

struct json_string_st {
  const char *s;
  size_t len;
};

struct json_int_st {
  int n;
};

struct json_double_st {
  double n;
};

struct json_value_st {
  json_value_type_t type;
  void *payload;
};

struct json_array_item_st {
  json_value_t *value;
  struct json_array_item_st *next;
  struct json_array_item_st *prev;
  json_array_t *parent;
};

struct json_array_st {
  size_t n;
  json_array_item_t *first_item;
};

struct json_kv_st {
  json_string_t *key;
  json_value_t *value;
  struct json_kv_st *next;
  struct json_kv_st *prev;
  json_object_t *parent;
};

struct json_object_st {
  size_t parent;
  size_t n;
  json_kv_t *first_kv;
};

struct json_st {
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
};

#endif
