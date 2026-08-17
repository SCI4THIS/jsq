#include "json_schema.h"
#include <string.h>
#include <stdio.h>

bool json_schema_validate(json_schema_t *schema, json_t *input)
{
}

json_schema_type_t json_schema_type_s(json_string_t *j_s)
{
  size_t len = json_string_len(j_s);
  const char *s = json_string_s(j_s);
  if (len == 8) {
    if (memcmp(s, "\"object\"", 8) == 0) {
      return JSON_SCHEMA_TYPE_OBJECT;
    }
    if (memcmp(s, "\"string\"", 8) == 0) {
      return JSON_SCHEMA_TYPE_STRING;
    }
  }
  return 0;
}

json_schema_type_t json_schema_type(json_value_t *v)
{
  json_value_type_t type = json_value_type(v);
  if (type == JSON_VALUE_TYPE_STRING) {
    return json_schema_type_s(json_value_payload(v));
  }
  return 0;
}

size_t json_schema(json_t *j, json_schema_t *js)
{
  json_object_t *o;
  json_kv_t *kv;
  json_schema_type_t type;
  size_t siz = sizeof(json_schema_t);
  size_t i;
  size_t n_entries = 0;
  size_t n_strings = 0;
  size_t n_objects = 0;
  size_t n_kvs;

  if (j == NULL) { return 0; }

  n_kvs = json_n_kvs(j);

  for (i=0; i<n_kvs; i++) {
    json_kv_t *kv = json_kv_i(j, i);
    json_string_t *key = json_kv_key(kv);
    json_object_t *o = NULL;
    json_array_t *a = NULL;
    json_value_t *v = NULL;
    if (json_string_len(key) == 6) {
      if (memcmp(json_string_s(key), "\"type\"", 6) == 0) {
        v = json_kv_value(kv);
        type = json_schema_type(v);
        n_entries++;
	switch (type) {
          case JSON_SCHEMA_TYPE_OBJECT:
            n_objects++;
	    break;
	  case JSON_SCHEMA_TYPE_STRING:
	    n_strings++;
	    o = json_kv_parent(kv);
	    v = json_find_value(j, o);
	    o = NULL;
	    a = NULL;
	    json_find_parent(j, v, &o, &a);
	    if (o != NULL) {
              kv = json_object_find_kv_v(o, v);
	      key = json_kv_key(kv);
	      printf("%.*s", json_string_len(key), json_string_s(key));
	    }
	    break;
	}
	printf("  type: %x\n", type);
      }
    }
  }
  if (js) {
    js->n_entries = n_entries;
    js->n_strings = n_strings;
    js->n_objects = n_objects;
  }
  return siz;
}
