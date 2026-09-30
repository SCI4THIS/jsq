#ifndef SCHEMA_H__
#define SCHEMA_H__ 1

#include <json.h>

typedef enum {
  JSON_SCHEMA_ENTRY_TYPE_STRING  = (1 << 0),
  JSON_SCHEMA_ENTRY_TYPE_BOOLEAN = (1 << 1),
  JSON_SCHEMA_ENTRY_TYPE_NUMBER  = (1 << 2),
  JSON_SCHEMA_ENTRY_TYPE_NULL    = (1 << 3),
  JSON_SCHEMA_ENTRY_TYPE_OBJECT  = (1 << 4),
  JSON_SCHEMA_ENTRY_TYPE_ARRAY   = (1 << 5),
  JSON_SCHEMA_ENTRY_TYPE_INVALID = (1 << 7),
} json_schema_entry_type_t;

typedef enum {
  JSON_SCHEMA_STRING_FORMAT_ANY,
  JSON_SCHEMA_STRING_FORMAT_DATE,
} json_schema_string_format_t;

typedef struct json_schema_args_st {
  size_t n_entries;
  size_t n_strings;
  size_t n_objects;
  size_t n_stab;
} json_schema_args_t;

typedef struct json_schema_st         json_schema_t;

bool json_schema_validate(json_schema_t *schema, json_t *input);
size_t json_schema(json_t *schema, json_schema_args_t *args, json_schema_t *js);
void json_schema_print(json_schema_t *js);
size_t json_schema_write(json_schema_t *js, char *buf, size_t len);
size_t json_schema_true(json_schema_t *js);
const json_schema_args_t *json_schema_args(json_schema_t *js);
size_t json_schema_read(const char *buf, size_t len, json_schema_t *js, json_schema_args_t *tally_args);
size_t json_schema_siz(size_t n_schemas, json_schema_args_t *args);

#endif
