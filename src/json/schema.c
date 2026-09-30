#include "schema.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef struct json_schema_object_st  json_schema_object_t;
typedef struct json_schema_string_st  json_schema_string_t;
typedef struct json_schema_entry_st   json_schema_entry_t;

typedef enum {
  JSON_SCHEMA_TYPE_NORMAL = 0,
  JSON_SCHEMA_TYPE_TRUE = 1,
  JSON_SCHEMA_TYPE_FALSE = 2,
} json_schema_type_t;

struct json_schema_object_st {
  size_t n_kvs;
};

struct json_schema_string_st {
  json_schema_string_format_t format;
};

struct json_schema_entry_st {
  json_schema_entry_type_t   type;
  const char                *keyidx;
  size_t                     keyidx_len;
  char                      *abs_key;
  size_t                     abs_key_len;
  json_schema_entry_t       *parent;
  struct {
    json_schema_object_t *object;
    json_schema_string_t *string;
  } qualifiers;
};

struct json_schema_st {
  json_schema_type_t type;
  json_schema_args_t args;
  json_schema_entry_t *entries;
  json_schema_string_t *strings;
  json_schema_object_t *objects;
  char *stab;
  char buf[];
};

static bool is_valid_object(json_schema_object_t *js_o, json_object_t *j_o)
{
  return true;
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
      if ((e->type & JSON_SCHEMA_ENTRY_TYPE_OBJECT) == 0) {
        return false;
      }
      return is_valid_object(e->qualifiers.object, payload);
    case JSON_VALUE_TYPE_STRING:
      if ((e->type & JSON_SCHEMA_ENTRY_TYPE_STRING) == 0) {
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
  if (js->type == JSON_SCHEMA_TYPE_FALSE) {
    return false;
  }
  if (js->type == JSON_SCHEMA_TYPE_TRUE) {
    return true;
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

json_schema_entry_type_t json_schema_entry_type_s(json_string_t *j_s)
{
  size_t len = json_string_len(j_s);
  const char *s = json_string_s(j_s);
  if (len == 8) {
    if (memcmp(s, "\"object\"", 8) == 0) {
      return JSON_SCHEMA_ENTRY_TYPE_OBJECT;
    }
    if (memcmp(s, "\"string\"", 8) == 0) {
      return JSON_SCHEMA_ENTRY_TYPE_STRING;
    }
  }
  return 0;
}

json_schema_entry_type_t json_schema_entry_type(json_value_t *v)
{
  json_value_type_t type = json_value_type(v);
  if (type == JSON_VALUE_TYPE_STRING) {
    return json_schema_entry_type_s(json_value_payload(v));
  }
  return 0;
}

typedef struct {
  json_schema_args_t *args;
  json_string_t *key;
  json_object_t *o;
  json_object_t *parent;
  json_schema_entry_t *e;
  json_schema_t *js;
  size_t base_key_len;
} handle_args_t;

size_t json_schema_handle_entry(handle_args_t handle_args);

size_t json_schema_handle_object(handle_args_t handle_args)
{
  json_schema_args_t   *args            = handle_args.args;
  json_string_t        *key             = NULL;
  json_object_t        *o               = handle_args.o;
  json_object_t        *parent          = handle_args.parent;
  json_schema_entry_t  *e               = handle_args.e;
  json_schema_t        *js              = handle_args.js;
  size_t                base_key_len    = handle_args.base_key_len;
  size_t                siz             = 0;
  size_t                i               = 0;
  json_kv_t            *kv              = NULL;
  json_object_t        *properties      = NULL;
  json_object_t        *child           = NULL;
  json_value_t         *v               = NULL;
  json_schema_object_t *js_o            = NULL;
  handle_args_t         handle_args_sub = { 0 };

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
    kv                           = json_kv_i(properties, i);
    key                          = json_kv_key(kv);
    v                            = json_kv_value(kv);

    assert(json_value_type(v) == JSON_VALUE_TYPE_OBJECT);

    child                        = json_value_payload(v);
    handle_args_sub.args         = args;
    handle_args_sub.key          = key;
    handle_args_sub.o            = child;
    handle_args_sub.parent       = o;
    handle_args_sub.e            = e;
    handle_args_sub.js           = js;
    handle_args_sub.base_key_len = base_key_len;
    siz += json_schema_handle_entry(handle_args_sub);
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

size_t json_schema_handle_string(handle_args_t handle_args)
{
  json_schema_args_t   *args         = handle_args.args;
  json_string_t        *key          = NULL;
  json_object_t        *o            = handle_args.o;
  json_object_t        *parent       = handle_args.parent;
  json_schema_entry_t  *e            = handle_args.e;
  json_schema_t        *js           = handle_args.js;
  size_t                base_key_len = handle_args.base_key_len;
  json_kv_t            *kv           = NULL;
  json_string_t        *format       = NULL;
  json_value_t         *v            = NULL;
  json_schema_string_t *s            = NULL;

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

size_t json_schema_handle_entry(handle_args_t handle_args)
{
  json_schema_args_t       *args            = handle_args.args;
  json_string_t            *key             = handle_args.key;
  json_object_t            *o               = handle_args.o;
  json_object_t            *parent          = handle_args.parent;
  json_schema_entry_t      *parent_e        = handle_args.e;
  json_schema_t            *js              = handle_args.js;
  size_t                    base_key_len    = handle_args.base_key_len;
  size_t                    siz             = 0;
  json_kv_t                *kv              = NULL;
  json_schema_entry_type_t  type            = JSON_SCHEMA_ENTRY_TYPE_INVALID;
  json_value_t             *v               = NULL;
  json_schema_entry_t      *e               = NULL;
  size_t                    key_len         = 0;
  size_t                    abs_key_len     = 0;
  handle_args_t             handle_args_sub = { 0 };

  kv = json_kv_s(o, "\"type\"");
  v = json_kv_value(kv);
  type = json_schema_entry_type(v);
  key_len = json_string_len(key);
  if (js != NULL) {
    size_t i = args->n_entries;
    e = &js->entries[i];
    e->parent = parent_e;
    e->keyidx_len = key_len;
    e->keyidx = json_string_s(key);
    e->type = type;
  }
  abs_key_len = base_key_len + 1 + key_len;
  args->n_entries++;
  args->n_stab                += abs_key_len;
  handle_args_sub.args         = args;
  handle_args_sub.key          = key;
  handle_args_sub.o            = o;
  handle_args_sub.parent       = parent;
  handle_args_sub.e            = e;
  handle_args_sub.js           = js;
  handle_args_sub.base_key_len = abs_key_len;
  switch (type) {
    case JSON_SCHEMA_ENTRY_TYPE_OBJECT:
      siz += json_schema_handle_object(handle_args_sub);
      break;
    case JSON_SCHEMA_ENTRY_TYPE_STRING:
      siz += json_schema_handle_string(handle_args_sub);
      break;
  }
  return siz + sizeof(json_schema_entry_t) + abs_key_len;
}

void json_schema_print(json_schema_t *js)
{
  size_t i;
  printf("Json Schema @ %p\n", js);
  if (js == NULL) {
    return;
  }
  if (js->type == JSON_SCHEMA_TYPE_TRUE) {
    printf("always true\n");
    return;
  }
  if (js->type == JSON_SCHEMA_TYPE_FALSE) {
    printf("always false\n");
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
      case JSON_SCHEMA_ENTRY_TYPE_STRING:
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
      case JSON_SCHEMA_ENTRY_TYPE_OBJECT:
        o = e->qualifiers.object;
	printf("{ object: %zu }\n", o->n_kvs);
        break;
    }
  }
}

size_t json_schema_true(json_schema_t *js)
{
  if (js) {
    js->type = JSON_SCHEMA_TYPE_TRUE;
  }
  return sizeof(json_schema_t);
}

size_t json_schema(json_t *j, json_schema_args_t *args, json_schema_t *js)
{
  json_value_t  *v           = NULL;
  json_object_t *o           = NULL;
  size_t         siz         = sizeof(json_schema_t);
  handle_args_t  handle_args = { 0 };

  if (j == NULL) { return 0; }
  if (args == NULL) { return 0; }

  v = json_root(j);
  if (json_value_type(v) == JSON_VALUE_TYPE_TRUE) {
    if (js) {
      js->type = JSON_SCHEMA_TYPE_TRUE;
    }
    goto end;
  }
  if (json_value_type(v) == JSON_VALUE_TYPE_FALSE) {
    if (js) {
      js->type = JSON_SCHEMA_TYPE_FALSE;
    }
    goto end;
  }

  if (js) {
    size_t i = 0;
    js->type = JSON_SCHEMA_TYPE_NORMAL;
    js->entries = (json_schema_entry_t *)&js->buf[i];
    i += args->n_entries * sizeof(json_schema_entry_t);
    js->strings = (json_schema_string_t *)&js->buf[i];
    i += args->n_strings * sizeof(json_schema_string_t);
    js->objects = (json_schema_object_t *)&js->buf[i];
    i += args->n_objects * sizeof(json_schema_object_t);
    js->stab = (char *)&js->buf[i];
  }

  memset(args, 0, sizeof(*args));

  assert(json_value_type(v) == JSON_VALUE_TYPE_OBJECT);
  o = json_value_payload(v);
  handle_args.args         = args;
  handle_args.key          = NULL;
  handle_args.o            = o;
  handle_args.parent       = NULL;
  handle_args.e            = NULL;
  handle_args.js           = js;
  handle_args.base_key_len = 0;
  siz += json_schema_handle_entry(handle_args);

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
end:
  return siz;
}

size_t json_schema_siz(size_t n_schemas, json_schema_args_t *args)
{
  return n_schemas * sizeof(json_schema_t) +
         args->n_entries * sizeof(json_schema_entry_t) +
	 args->n_strings * sizeof(json_schema_string_t) +
	 args->n_objects * sizeof(json_schema_object_t) +
	 args->n_stab;
}

static json_schema_type_t json_schema_entry_read(const char *buf, size_t len, json_schema_entry_t *e, json_schema_args_t *args)
{
  size_t i = 0;
  const char *decstr = "0123456789";
  json_schema_type_t type;
  size_t keylen;
  if ((i + 9) >= len)              { goto err; }
  if (!strchr(decstr, buf[i+0]))   { goto err; }
  if (!strchr(decstr, buf[i+1]))   { goto err; }
  if (!strchr(decstr, buf[i+2]))   { goto err; }
  if (buf[i+3] != ' ')             { goto err; }
  if (!strchr(decstr, buf[i+4]))   { goto err; }
  if (!strchr(decstr, buf[i+5]))   { goto err; }
  if (!strchr(decstr, buf[i+6]))   { goto err; }
  if (!strchr(decstr, buf[i+7]))   { goto err; }
  if (buf[i+8] != ' ')             { goto err; }
  sscanf(buf, "%03d %04d ", &type, &keylen);

  i = 9;
  if ((i + keylen) >= len)         { goto err; }
  i += keylen;

  if (type & JSON_SCHEMA_ENTRY_TYPE_STRING) {
    json_schema_string_format_t format;
    if ((i + 3) >= len)            { goto err; }
    if (buf[i] != ' ')             { goto err; }
    i++;
    if (!strchr(decstr, buf[i+0])) { goto err; }
    if (!strchr(decstr, buf[i+1])) { goto err; }
    if (strchr(decstr, buf[i+2]))  { goto err; }
    sscanf(&buf[i], "%02d", &format);
    i += 2;
    if (e) {
      e->qualifiers.string->format = format;
    }
  }

  if (i != len - 1)                { goto err; }
  if (buf[i] != '\n')              { goto err; }

  if (e) {
    e->type = type;
    e->abs_key_len = keylen;
    if (keylen > 0) {
      memcpy(e->abs_key, &buf[9], keylen);
    }
  }

  args->n_entries++;
  if (type & JSON_SCHEMA_ENTRY_TYPE_STRING) { args->n_strings++; }
  if (type & JSON_SCHEMA_ENTRY_TYPE_OBJECT) { args->n_objects++; }
  args->n_stab += keylen;

  return type;
err:
  return JSON_SCHEMA_ENTRY_TYPE_INVALID;
}

size_t json_schema_read(const char *buf, size_t len, json_schema_t *js)
{
  size_t i = 0;
  size_t _i;
  size_t i_entries = 0;
  size_t i_strings = 0;
  size_t i_objects = 0;
  size_t js_pos = 0;
  size_t stab_i = 0;
  const char s_type[] = "type";
  const char s_n_entries[] = "n_entries";
  const char s_n_strings[] = "n_strings";
  const char s_n_objects[] = "n_objects";
  const char s_n_stab[] = "n_stab";
  size_t type;
  json_schema_args_t args = { 0 };
  json_schema_args_t tally_args = { 0 };

#define PARSE(s, v) \
  i += sizeof(s) - 1; \
  sscanf(&buf[i], " = %zu", &(v)); \
  while (i < len && buf[i++] != '\n');

#define CHECK(s, v) \
  _i = i; \
  if ((i + sizeof(s) - 1) >= len) { return 0; } \
  if (memcmp(&buf[i], s, sizeof(s) - 1) != 0) { return 0; } \
  i += sizeof(s) - 1; \
  if (memcmp(&buf[i], " = ", 3) != 0) { return 0; } \
  i += 3; \
  if (!strchr("0123456789", buf[i])) { return 0; } \
  while (i < len && strchr("0123456789", buf[i++])); \
  i = _i; \
  PARSE(s, v)

  if (!js) {
    CHECK(s_type, type);
    CHECK(s_n_entries, args.n_entries);
    CHECK(s_n_strings, args.n_strings);
    CHECK(s_n_objects, args.n_objects);
    CHECK(s_n_stab, args.n_stab);
  } else {
    PARSE(s_type, type);
    PARSE(s_n_entries, args.n_entries);
    PARSE(s_n_strings, args.n_strings);
    PARSE(s_n_objects, args.n_objects);
    PARSE(s_n_stab, args.n_stab);
  }

  if (js) {
    size_t js_pos = 0;
    js->type = (json_schema_type_t)type;
    js->entries = (json_schema_entry_t *)&js->buf[js_pos];
    js_pos += args.n_entries * sizeof(json_schema_entry_t);
    js->strings = (json_schema_string_t *)&js->buf[js_pos];
    js_pos += args.n_strings * sizeof(json_schema_string_t);
    js->objects = (json_schema_object_t *)&js->buf[js_pos];
    js_pos += args.n_objects * sizeof(json_schema_object_t);
    js->stab = (char *)&js->buf[js_pos];
    memcpy(&js->args, &args, sizeof(args));
  }

  while (i<len) {
    json_schema_entry_t *e = NULL;
    size_t start_i = i;
    json_schema_entry_type_t type;
    while (i<len && buf[i++] != '\n');
    if (js) {
      e = &js->entries[tally_args.n_entries];
      e->qualifiers.string = &js->strings[tally_args.n_strings];
      e->qualifiers.object = &js->objects[tally_args.n_objects];
      e->abs_key = &js->stab[tally_args.n_stab];
    }
    type = json_schema_entry_read(&buf[start_i], i - start_i, e, &tally_args);
    if (type == JSON_SCHEMA_ENTRY_TYPE_INVALID) {
      return 0;
    }
    if (e) {
      if (e->abs_key_len == 0) {
        e->abs_key = NULL;
      }
      if (!(type & JSON_SCHEMA_ENTRY_TYPE_STRING)) {
        e->qualifiers.string = NULL;
      }
      if (!(type & JSON_SCHEMA_ENTRY_TYPE_OBJECT)) {
        e->qualifiers.object = NULL;
      }
    }
  }

  if (tally_args.n_stab < args.n_stab) {
    tally_args.n_stab = args.n_stab;
  }
  if (memcmp(&tally_args, &args, sizeof(args)) != 0) { return 0; }

  return json_schema_siz(1, &args);
}

size_t json_schema_write(json_schema_t *js, char *buf, size_t len)
{
  size_t              siz = 0;
  size_t              i   = 0;
  char              *_buf = buf;
  size_t             _len = len;
  json_schema_type_t type = JSON_SCHEMA_TYPE_NORMAL;
  size_t             n    = 0;

#define ADVANCE() if (buf) { _buf = &buf[siz]; _len = len - siz; }

  siz += snprintf(_buf, _len, "type = %x\n", js->type);
  ADVANCE()
  siz += snprintf(_buf, _len, "n_entries = %zu\n", js->args.n_entries);
  ADVANCE()
  siz += snprintf(_buf, _len, "n_strings = %zu\n", js->args.n_strings);
  ADVANCE()
  siz += snprintf(_buf, _len, "n_objects = %zu\n", js->args.n_objects);
  ADVANCE()
  siz += snprintf(_buf, _len, "n_stab = %zu\n", js->args.n_stab);
  ADVANCE()
  n = js->args.n_entries;
  for (i=0; i<n; i++) {
    json_schema_entry_t *e = &js->entries[i];
    json_schema_entry_type_t e_t = e->type;
    size_t key_len = e->abs_key_len;
    const char *key = e->abs_key;
    siz += snprintf(_buf, _len, "%03d %04zu %.*s", e_t, key_len, key_len, key);
    ADVANCE()
    if (e_t & JSON_SCHEMA_ENTRY_TYPE_STRING) {
      json_schema_string_format_t format;
      format = e->qualifiers.string->format;
      siz += snprintf(_buf, _len, " %02d", format);
      ADVANCE()
    }
    siz += snprintf(_buf, _len, "\n");
    ADVANCE()
  }
  return siz;

#undef ADVANCE
}

const json_schema_args_t *json_schema_args(json_schema_t *js)
{
  if (!js) {
    return NULL;
  }
  return &js->args;
}

