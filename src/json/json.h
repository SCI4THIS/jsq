#ifndef JSON_H__
#define JSON_H__ 1

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

typedef struct json_object_st     json_object_t;
typedef struct json_string_st     json_string_t;
typedef struct json_int_st        json_int_t;
typedef struct json_double_st     json_double_t;
typedef struct json_value_st      json_value_t;
typedef struct json_array_st      json_array_t;
typedef struct json_array_item_st json_array_item_t;
typedef struct json_kv_st         json_kv_t;
typedef struct json_st            json_t;

void               json_args_print(json_args_t *args);
size_t             json(json_parser_t *p, json_t *j);
void               json_print(json_t *j);
void               json_defrag(json_t *j);
json_object_t     *json_root(json_t *j);
json_kv_t         *json_kv(json_object_t *o, const char *key, size_t len);
json_kv_t         *json_kv_s(json_object_t *o, const char *key);
json_string_t     *json_kv_key(json_kv_t *kv);
json_object_t     *json_kv_parent(json_kv_t *kv);
json_value_t      *json_kv_value(json_kv_t *kv);
void               json_print_key_val(json_t *j, json_value_t *v);
size_t             json_string_len(json_string_t *s);
const char        *json_string_s(json_string_t *s);
size_t             json_n_kvs(json_t *j);
json_kv_t         *json_kv_i(json_t *j, size_t i);
json_value_type_t  json_value_type(json_value_t *v);
void              *json_value_payload(json_value_t *v);
json_value_t      *json_find_value(json_t *j, void *o);
json_kv_t         *json_object_find_kv_v(json_object_t *o, json_value_t *v);
void              *json_find_parent(json_t *j, json_value_t *v,
                                    json_object_t **o, json_array_t **a);

#endif
