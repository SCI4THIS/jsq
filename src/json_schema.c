#include "json_schema.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

struct json_schema_object_st {
  size_t n_kvs;
};

struct json_schema_string_st {
  json_schema_string_format_t format;
};

struct json_schema_entry_st {
  json_schema_type_t type;
  const char *keyidx;
  size_t keyidx_len;
  char *abs_key;
  size_t abs_key_len;
  json_schema_entry_t *parent;
  struct {
    json_schema_object_t *object;
    json_schema_string_t *string;
  } qualifiers;
};

struct json_schema_st {
  json_schema_args_t args;
  json_schema_entry_t *entries;
  json_schema_string_t *strings;
  json_schema_object_t *objects;
  char *stab;
  char buf[];
};

static bool is_valid_object(json_schema_object_t *js_o, json_object_t *j_o)
{
  return js_o->n_kvs == json_n_kvs(j_o);
}

static bool is_valid_string(json_schema_string_t *js_s, json_string_t *j_s)
{
  if (js_s->format == JSON_SCHEMA_STRING_FORMAT_ANY) {
    return true;
  }
  if (js_s->format == JSON_SCHEMA_STRING_FORMAT_DATE) {
    /* YYYY-MM-DD */
    size_t digit_idx[] = { 1, 2, 3, 4, 6, 7, 9, 10 };
    size_t hyphon_idx[] = { 5, 8 };
    size_t quote_idx[] = { 0, 11 };
    size_t i;
    const char *s;
    if (json_string_len(j_s) != 12) {
      return false;
    }
    s = json_string_s(j_s);
    for (i=0; i<(sizeof(digit_idx)/sizeof(digit_idx[0])); i++) {
      const char c = s[digit_idx[i]];
      if ('0' <= c && c <= '9') {
        continue;
      }
      return false;
    }
    for (i=0; i<(sizeof(hyphon_idx)/sizeof(hyphon_idx[0])); i++) {
      const char c = s[hyphon_idx[i]];
      if (c == '-') {
        continue;
      }
      return false;
    }
    for (i=0; i<(sizeof(quote_idx)/sizeof(quote_idx[0])); i++) {
      const char c = s[quote_idx[i]];
      if (c == '"') {
        continue;
      }
      return false;
    }
    return true;
  }
  return false;
}

static bool is_valid_entry(json_schema_entry_t *e, json_value_t *v)
{
  json_value_type_t j_t = json_value_type(v);
  void *payload = json_value_payload(v);
  switch (j_t) {
    case JSON_VALUE_TYPE_OBJECT:
      if ((e->type & JSON_SCHEMA_TYPE_OBJECT) == 0) {
        return false;
      }
      return is_valid_object(e->qualifiers.object, payload);
    case JSON_VALUE_TYPE_STRING:
      if ((e->type & JSON_SCHEMA_TYPE_STRING) == 0) {
        return false;
      }
      return is_valid_string(e->qualifiers.string, payload);
    default:
      break;
  }
  return false;
}

bool json_schema_validate(json_schema_t *js, json_t *j)
{
  size_t i;
  if (js == NULL) {
    return false;
  }
  if (j == NULL) {
    return false;
  }
  for (i=0; i<js->args.n_entries; i++) {
    json_schema_entry_t *e = &js->entries[i];
    json_value_t *v = json_value(j, e->abs_key, e->abs_key_len);
    if (!is_valid_entry(e, v)) {
      return false;
    }
  }
  return true;
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

size_t json_schema_handle_entry(json_schema_args_t *args, json_string_t *key, json_object_t *o, json_object_t *parent, json_schema_entry_t *e, json_schema_t *js, size_t base_key_len);

size_t json_schema_handle_object(json_schema_args_t *args, json_object_t *o, json_object_t *parent, json_schema_entry_t *e, json_schema_t *js, size_t base_key_len)
{
  size_t siz = 0;
  size_t i;
  json_kv_t *kv;
  json_string_t *key;
  json_object_t *properties;
  json_object_t *child;
  json_value_t *v;
  json_schema_object_t *js_o = NULL;

  kv = json_kv_s(o, "\"properties\"");
  v = json_kv_value(kv);
  assert(json_value_type(v) == JSON_VALUE_TYPE_OBJECT);
  properties = json_value_payload(v);
  if (js != NULL) {
    size_t i = args->n_objects;
    js_o = &js->objects[i];
    e->qualifiers.object = js_o;
  }
  args->n_objects++;
  for (i=0; i<json_n_kvs(properties); i++) {
    kv = json_kv_i(properties, i);
    key = json_kv_key(kv);
    v = json_kv_value(kv);
    assert(json_value_type(v) == JSON_VALUE_TYPE_OBJECT);
    child = json_value_payload(v);
    siz += json_schema_handle_entry(args, key, child, o, e, js, base_key_len);
  }
  if (js_o) {
    js_o->n_kvs = json_n_kvs(properties);
  }
  return siz + sizeof(json_schema_object_t);
}

size_t json_schema_abs_key(json_schema_entry_t *e, char *s)
{
  size_t i = 0;
  if (e->parent == NULL) {
    return 0;
  }
  i = json_schema_abs_key(e->parent, s);
  s[i] = '.';
  i++;
  memmove(&s[i], e->keyidx, e->keyidx_len);
  return i + e->keyidx_len;
}

size_t json_schema_handle_string(json_schema_args_t *args, json_object_t *o, json_object_t *parent, json_schema_entry_t *e, json_schema_t *js, size_t base_key_len)
{
  json_kv_t *kv;
  json_string_t *format;
  json_value_t *v;
  json_schema_string_t *s = NULL;

  if (js != NULL) {
    size_t i = args->n_strings;
    s = &js->strings[i];
    s->format = JSON_SCHEMA_STRING_FORMAT_ANY;
    e->qualifiers.string = s;
  }
  args->n_strings++;
  kv = json_kv_s(o, "\"format\"");
  if (kv != NULL) {
    v = json_kv_value(kv);
    assert(json_value_type(v) == JSON_VALUE_TYPE_STRING);
    format = json_value_payload(v);
    if (s != NULL && json_string_eq_s(format, "\"date\"")) {
      s->format = JSON_SCHEMA_STRING_FORMAT_DATE;
    }
  }
  return sizeof(json_schema_string_t);
}

size_t json_schema_handle_entry(json_schema_args_t *args, json_string_t *key, json_object_t *o, json_object_t *parent, json_schema_entry_t *parent_e, json_schema_t *js, size_t base_key_len)
{
  size_t siz = 0;
  json_kv_t *kv;
  json_schema_type_t type;
  json_value_t *v;
  json_schema_entry_t *e;
  size_t key_len;

  kv = json_kv_s(o, "\"type\"");
  v = json_kv_value(kv);
  type = json_schema_type(v);
  key_len = json_string_len(key);
  if (js != NULL) {
    size_t i = args->n_entries;
    e = &js->entries[i];
    e->parent = parent_e;
    e->keyidx_len = key_len;
    e->keyidx = json_string_s(key);
    e->type = type;
  }
  args->n_entries++;
  args->n_stab += base_key_len + 1 + key_len;
  switch (type) {
    case JSON_SCHEMA_TYPE_OBJECT:
      siz += json_schema_handle_object(args, o, parent, e, js, base_key_len + 1 + key_len);
      break;
    case JSON_SCHEMA_TYPE_STRING:
      siz += json_schema_handle_string(args, o, parent, e, js, base_key_len + 1 + key_len);
      break;
  }
  return siz + sizeof(json_schema_entry_t) + base_key_len + 1 + key_len;
}

void json_schema_print(json_schema_t *js)
{
  size_t i;
  printf("Json Schema @ %p\n", js);
  if (js == NULL) {
    return;
  }
  printf("n_entries: %zu\n", js->args.n_entries);
  printf("n_strings: %zu\n", js->args.n_strings);
  printf("n_objects: %zu\n", js->args.n_objects);
  printf("n_stab: %zu\n", js->args.n_stab);
  for (i=0; i<js->args.n_entries; i++) {
    json_schema_entry_t *e = &js->entries[i];
    json_schema_string_t *s = NULL;
    json_schema_object_t *o = NULL;
    if (e->abs_key_len == 0) {
      printf(".: ");
    } else {
      printf("%.*s: ", e->abs_key_len, e->abs_key);
    }
    switch (e->type) {
      case JSON_SCHEMA_TYPE_STRING:
        s = e->qualifiers.string;
        switch (s->format) {
          case JSON_SCHEMA_STRING_FORMAT_ANY:
            printf("{ string: any }\n");
            break;
          case JSON_SCHEMA_STRING_FORMAT_DATE:
            printf("{ string: date }\n");
	    break;
	}
	break;
      case JSON_SCHEMA_TYPE_OBJECT:
        o = e->qualifiers.object;
	printf("{ object: %zu }\n", o->n_kvs);
        break;
    }
  }
}

size_t json_schema(json_t *j, json_schema_args_t *args, json_schema_t *js)
{
  json_value_t *v;
  json_object_t *o;
  size_t siz = sizeof(json_schema_t);

  if (j == NULL) { return 0; }
  if (args == NULL) { return 0; }
  if (js) {
    size_t i = 0;
    js->entries = (json_schema_entry_t *)&js->buf[i];
    i += args->n_entries * sizeof(json_schema_entry_t);
    js->strings = (json_schema_string_t *)&js->buf[i];
    i += args->n_strings * sizeof(json_schema_string_t);
    js->objects = (json_schema_object_t *)&js->buf[i];
    i += args->n_objects * sizeof(json_schema_object_t);
    js->stab = (char *)&js->buf[i];
  }

  memset(args, 0, sizeof(*args));

  v = json_root(j);
  assert(json_value_type(v) == JSON_VALUE_TYPE_OBJECT);
  o = json_value_payload(v);
  siz += json_schema_handle_entry(args, NULL, o, NULL, NULL, js, 0);

  if (js) {
    size_t i;
    size_t stab_i = 0;
    size_t abs_key_len;
    char *abs_key;
    memmove(&js->args, args, sizeof(*args));
    for (i=0; i<args->n_entries; i++) {
      json_schema_entry_t *e = &js->entries[i];
      e->abs_key = &js->stab[stab_i];
      e->abs_key_len += json_schema_abs_key(e, e->abs_key);
      stab_i += e->abs_key_len;
      assert(stab_i <= js->args.n_stab);
    }
  }
  return siz;
}
