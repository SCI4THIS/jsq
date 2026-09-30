#include "harness.h"
#include <string.h>

struct json_schema_harness_st {
  size_t n;
  json_schema_t **js;
  char buf[];
};

void json_schema_harness_print(json_schema_harness_t *jsh)
{
  size_t i;
  size_t n;
  printf("Json Schema Harness @ %p\n", jsh);
  if (jsh == NULL) {
    return;
  }
  n = jsh->n;
  for (i=0; i<n; i++) {
    printf("\n[%d]\n\n", i);
    json_schema_print(jsh->js[i]);
  }
}

const char jsh_magic[] = "JSQv1";
#define JSH_MAGIC_LEN (sizeof(jsh_magic) - 1)

size_t json_schema_harness_read(const char *buf, size_t len, json_schema_harness_t *jsh)
{
  size_t i;
  size_t _i;
  size_t j = 0;
  size_t start_i;
  size_t siz;
  char *s;
  size_t jsh_pos = 0;
  const char s_n_schemas[] = "n_schemas";
  const char s_tot_entries[] = "tot_entries";
  const char s_tot_strings[] = "tot_strings";
  const char s_tot_objects[] = "tot_objects";
  const char s_tot_stab[] = "tot_stab";
  size_t n_schemas;
  json_schema_args_t args = { 0 };
  size_t sub_len;

  if (memcmp(jsh_magic, buf, JSH_MAGIC_LEN) != 0) { return 0; }
  i = JSH_MAGIC_LEN;
  if (buf[i] != '\n') { return 0; }
  i++;

#define PARSE(s, v) \
  i += sizeof(s) - 1; \
  sscanf(&buf[i], " = %zu", &(v)); \
  while (buf[i++] != '\n');

#define CHECK(s, v) \
  _i = i; \
  if ((i + sizeof(s) - 1) >= len) { return 0; } \
  if (memcmp(&buf[i], s, sizeof(s) - 1) != 0) { return 0; } \
  i += sizeof(s) - 1; \
  if (memcmp(&buf[i], " = ", 3) != 0) { return 0; } \
  i += 3; \
  if (!strchr("0123456789", buf[i])) { return 0; } \
  while (strchr("0123456789", buf[i++])); \
  i = _i; \
  PARSE(s, v)

  if (!jsh) {
    CHECK(s_n_schemas, n_schemas);
    CHECK(s_tot_entries, args.n_entries);
    CHECK(s_tot_strings, args.n_strings);
    CHECK(s_tot_objects, args.n_objects);
    CHECK(s_tot_stab, args.n_stab);
  } else {
    PARSE(s_n_schemas, n_schemas);
    PARSE(s_tot_entries, args.n_entries);
    PARSE(s_tot_strings, args.n_strings);
    PARSE(s_tot_objects, args.n_objects);
    PARSE(s_tot_stab, args.n_stab);
  }

  if (jsh) {
    jsh->n = n_schemas;
    jsh_pos = 0;
    jsh->js = (json_schema_t **)&jsh->buf[jsh_pos];
    jsh_pos += sizeof(json_schema_t *) * n_schemas;
  }

  if (buf[i] != '\n') { return 0; }
  i++;
  start_i = i;
  s = strstr(&buf[i], "\n\n");
  if (s) {
    sub_len = s - &buf[start_i];
  } else {
    sub_len = len - start_i;
  }
  do {
    json_schema_t *js = NULL;
    start_i = i;
    s = strstr(&buf[i], "\n\n");
    if (s) {
      sub_len = s - &buf[start_i];
    } else {
      sub_len = len - start_i - 1;
    }
    if (jsh) {
      js = (json_schema_t *)&jsh->buf[jsh_pos];
      jsh->js[j] = js;
      j++;
    }
    siz = json_schema_read(&buf[start_i], sub_len, js);
    if (siz == 0) { return 0; }
    jsh_pos += siz;
    i += sub_len + 2;
  } while (s != NULL);

end:
  return
    sizeof(json_schema_harness_t) +
    args.n_entries * sizeof(json_schema_t *) +
    json_schema_siz(n_schemas, &args);
}

size_t json_schema_harness_write(json_schema_harness_t *jsh, char *buf, size_t len)
{
  size_t   siz       = 0;
  size_t   i         = 0;
  size_t   n         = 0;
  char    *_buf      = buf;
  size_t   _len      = len;
  size_t   n_entries = 0;
  size_t   n_strings = 0;
  size_t   n_objects = 0;
  size_t   n_stab    = 0;

#define ADVANCE() if (buf) { _buf = &buf[siz]; _len = len - siz; }

  if (jsh == NULL) { return 0; }

  n = jsh->n;
  siz += snprintf(_buf, _len, "%s\n", jsh_magic);
  ADVANCE()
  for (i=0; i<n; i++) {
    json_schema_t *js = jsh->js[i];
    const json_schema_args_t *js_args = json_schema_args(js);
    n_entries += js_args->n_entries;
    n_strings += js_args->n_strings;
    n_objects += js_args->n_objects;
    n_stab    += js_args->n_stab;
  }
  siz += snprintf(_buf, _len, "n_schemas = %zu\n", n);
  ADVANCE()
  siz += snprintf(_buf, _len, "tot_entries = %zu\n", n_entries);
  ADVANCE()
  siz += snprintf(_buf, _len, "tot_strings = %zu\n", n_strings);
  ADVANCE()
  siz += snprintf(_buf, _len, "tot_objects = %zu\n", n_objects);
  ADVANCE()
  siz += snprintf(_buf, _len, "tot_stab = %zu\n", n_stab);
  ADVANCE()
  for (i=0; i<n; i++) {
    json_schema_t *js = jsh->js[i];
    siz += snprintf(_buf, _len, "\n");
    ADVANCE()
    siz += json_schema_write(js, _buf, _len);
    ADVANCE()
  }
  siz += snprintf(_buf, _len, "\n");
  ADVANCE()
  return siz;

#undef ADVANCE
}

size_t json_schema_harness(size_t n, json_t **j, json_schema_args_t *args, json_schema_harness_t *jsh)
{
  size_t i;
  char *data = NULL;
  size_t siz = 0;
  json_schema_t *js = NULL;
  if (j == NULL) { return 0; }
  if (args == NULL) { return 0; }
  if (jsh) {
    jsh->n = n + 1;
    jsh->js = (json_schema_t **)&jsh->buf[0];
    data = &jsh->buf[jsh->n * sizeof(json_schema_t *)];
  }
  if (jsh) {
      js = (json_schema_t *)&data[siz];
      jsh->js[0] = js;
  }
  siz += json_schema_true(js);
  for (i=0; i<n; i++) {
    if (jsh) {
      js = (json_schema_t *)&data[siz];
      jsh->js[i+1] = js;
    }
    siz += json_schema(j[i], &args[i], js);
  }
  siz += sizeof(json_schema_harness_t) + (n + 1) * sizeof(json_schema_t *);
  return siz;
}

json_schema_t *json_schema_harness_schema(json_schema_harness_t *jsh, size_t i)
{
  if (jsh == NULL) { return NULL; }
  if (i >= jsh->n) { return NULL; }
  return jsh->js[i];
}

size_t json_schema_harness_classify(json_schema_harness_t *jsh, json_t *j)
{
  size_t i = 0;
  size_t n = jsh->n;
  for (i=n; i-->0;) {
    if (json_schema_validate(jsh->js[i], j)) {
      return i;
    }
  }
  return 0;
}
