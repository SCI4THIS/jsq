#ifndef JSON_SCHEMA_H__
#define JSON_SCHEMA_H__ 1

#include <json.h>
/*
{
  "type": "object",
  "properties": {
    "first_name": { "type": "string" },
    "last_name": { "type": "string" },
    "birthday": { "type": "string", "format": "date" },
    "address": {
       "type": "object",
       "properties": {
         "street_address": { "type": "string" },
         "city": { "type": "string" },
         "state": { "type": "string" },
         "country": { "type" : "string" }
       }
    }
  }
}
*/

typedef enum {
  JSON_SCHEMA_TYPE_STRING  = (1 << 0),
  JSON_SCHEMA_TYPE_BOOLEAN = (1 << 1),
  JSON_SCHEMA_TYPE_NUMBER  = (1 << 2),
  JSON_SCHEMA_TYPE_NULL    = (1 << 3),
  JSON_SCHEMA_TYPE_OBJECT  = (1 << 4),
  JSON_SCHEMA_TYPE_ARRAY   = (1 << 5),
} json_schema_type_t;

typedef enum {
  JSON_SCHEMA_STRING_FORMAT_ANY,
  JSON_SCHEMA_STRING_FORMAT_DATE,
} json_schema_string_format_t;

typedef struct json_schema_object_st {
  size_t n_kvs;
} json_schema_object_t;

typedef struct json_schema_string_st {
  json_schema_string_format_t format;
} json_schema_string_t;

typedef struct json_schema_entry_st {
  json_schema_type_t type;
  const char *keyidx;
  size_t keyidx_len;
  void *payload;
} json_schema_entry_t;

typedef struct json_schema_st {
  size_t n_entries;
  size_t n_strings;
  size_t n_objects;
  json_schema_entry_t *entries;
  json_schema_string_t *strings;
  json_schema_object_t *objects;
  const char *stab;
  char buf[];
} json_schema_t;

bool json_schema_validate(json_schema_t *schema, json_t *input);
size_t json_schema(json_t *schema, json_schema_t *js);

#endif
