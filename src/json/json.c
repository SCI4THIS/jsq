#include <stdio.h>
#include <json.h>
#include <string.h>

void *yy_scan_buffer  (char * base, size_t  size );
void *yy_scan_bytes ( const char *bytes, int len  );
void  yy_delete_buffer (void *);
void yy_set_parser(json_parser_t *p);
void yy_set_json(json_t *j);
int  yylex_destroy(void);
extern int yyparse(void);
extern int line;

json_value_t *json_find_value(json_t *j, json_object_t *o);
json_object_t *json_find_root(json_t *j);

void json_args_print(json_args_t *args)
{
  printf("JSON ARGS @ %p\n", args);
  printf("n_strings: %zu\n", args->n_strings);
  printf("n_objects: %zu\n", args->n_objects);
  printf("n_values: %zu\n", args->n_values);
  printf("n_kvs: %zu\n", args->n_kvs);
  printf("n_arrays: %zu\n", args->n_arrays);
  printf("n_ints: %zu\n", args->n_ints);
  printf("n_doubles: %zu\n", args->n_doubles);
  printf("n_array_items: %zu\n", args->n_array_items);
  printf("n_stab: %zu\n", args->n_stab);
}

void json_print_debug(json_t *j)
{
  size_t i;
  printf("JSON @ %p\n", j);
  if (j == NULL) {
    return;
  }
  printf("n_strings: %zu\n", j->args.n_strings);
  for (i=0; i<j->args.n_strings; i++) {
    const char *s = j->strings[i].s;
    size_t len = j->strings[i].len;
    printf("strings[%zu]: %.*s\n", i, len, s);
  }
  printf("n_objects: %zu\n", j->args.n_objects);
  for (i=0; i<j->args.n_objects; i++) {
    json_object_t *o = &j->objects[i];
    printf("objects[%zu]: %p { n = %zu, first_kv = %p }\n", i, o, o->n, o->first_kv);
  }
  printf("n_values: %zu\n", j->args.n_values);
  for (i=0; i<j->args.n_values; i++) {
    json_value_t *v = &j->values[i];
    printf("values[%zu]: %p { t = %d, payload = %p }\n", i, v, v->type, v->payload);
  }
  printf("n_kvs: %zu\n", j->args.n_kvs);
  for (i=0; i<j->args.n_kvs; i++) {
    json_kv_t *kv = &j->kvs[i];
    printf("kvs[%zu]: %p { key = %p, value = %p, prev = %p, next = %p }\n", i, kv, kv->key, kv->value, kv->prev, kv->next);
  }
  printf("n_arrays: %zu\n", j->args.n_arrays);
  for (i=0; i<j->args.n_arrays; i++) {
    json_array_t *a = &j->arrays[i];
    printf("arrays[%zu]: %p { n = %zu, first_item = %p }\n", i, a, a->n, a->first_item);
  }
  printf("n_ints: %zu\n", j->args.n_ints);
  for (i=0; i<j->args.n_ints; i++) {
    json_int_t *n = &j->ints[i];
    printf("ints[%zu]: %p { n = %d }\n", i, n, n->n);
  }
  printf("n_doubles: %zu\n", j->args.n_doubles);
  for (i=0; i<j->args.n_doubles; i++) {
    json_double_t *n = &j->doubles[i];
    printf("doubles[%zu]: %p { n = %lf }\n", i, n, n->n);
  }
  printf("n_array_items: %zu\n", j->args.n_array_items);
  for (i=0; i<j->args.n_array_items; i++) {
    json_array_item_t *item = &j->array_items[i];
    printf("array_items[%zu]: %p { val = %p, prev = %p, next = %p }\n", i, item, item->value, item->prev, item->next);
  }
}

void json_print_object(json_object_t *o)
{
  size_t i;
  json_kv_t *kv = o->first_kv;

  while (kv != NULL) {
    printf("%.*s:\n", kv->key->len, kv->key->s);
    kv = kv->next;
  }
}

void json_print_string(json_string_t *s)
{
  printf("%.*s", s->len, s->s);
}

void json_print_int(json_int_t *i)
{
  printf("%d", i->n);
}

void json_print_double(json_double_t *d)
{
  printf("%lf", d->n);
}

void json_print_key_val(json_t *j, json_value_t *v)
{
  size_t i;
  for (i=0; i<j->args.n_kvs; i++) {
    if (v == j->kvs[i].value) {
      json_object_t *o = j->kvs[i].parent;
      json_print_key_val(j, json_find_value(j, o));
      printf(".%.*s", j->kvs[i].key->len, j->kvs[i].key->s, o);
      break;
    }
  }
}

void json_print(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_values; i++) {
    json_value_t *v = &j->values[i];
    switch (v->type) {
      case JSON_VALUE_TYPE_INVALID:
      case JSON_VALUE_TYPE_OBJECT:
      case JSON_VALUE_TYPE_ARRAY:
        continue;
    }
    json_print_key_val(j, v);
    printf(": ");
    switch (v->type) {
      case JSON_VALUE_TYPE_INT:
        json_print_int(v->payload);
	break;
      case JSON_VALUE_TYPE_DOUBLE:
	json_print_double(v->payload);
	break;
      case JSON_VALUE_TYPE_TRUE:
	printf("true");
	break;
      case JSON_VALUE_TYPE_FALSE:
	printf("false");
	break;
      case JSON_VALUE_TYPE_STRING:
	json_print_string(v->payload);
	break;
      case JSON_VALUE_TYPE_NULL:
	printf("null");
	break;
    }
    printf("\n");
  }
}

json_value_t *json_find_value(json_t *j, json_object_t *o)
{
  size_t i;
  for (i=0; i<j->args.n_values; i++) {
    json_value_t *v = &j->values[i];
    if (v->payload == o) {
      return v;
    }
  }
  return NULL;
}

json_object_t *json_find_root(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_objects; i++) {
    json_object_t *o = &j->objects[i];
    if (json_find_value(j, o) == NULL) {
      return o;
    }
  }
  return NULL;
}

size_t json_assign_entries(json_parser_t *p, json_t *j)
{
  size_t i = 0;

  memmove(&j->args, &p->args, sizeof(json_args_t));
#define ENTRY(v) \
  j->v##s = (json_##v##_t *)&j->buf[i]; \
  i += sizeof(json_##v##_t) * p->args.n_##v##s;

  ENTRY(string);
  ENTRY(object);
  ENTRY(value);
  ENTRY(kv);
  ENTRY(array);
  ENTRY(int);
  ENTRY(double);
  ENTRY(array_item);

#undef ENTRY
  j->stab = (char *)&j->buf[i];
  i += p->args.n_stab;

  return i + sizeof(json_t);
}

size_t json(json_parser_t *p, json_t *j)
{
  json_t dummy;
  void *yy_buffer_state = NULL;

  yy_set_parser(p);
  yy_set_json(j);

  if (j == NULL) {
    j = &dummy;
  }

  json_assign_entries(p, j);
  json_parser_reset(p);
  yy_buffer_state = yy_scan_bytes(p->s, p->siz);
  line = 1;
  yyparse();
  yylex_destroy();

  return json_assign_entries(p, j);
}

