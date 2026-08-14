#include <stdio.h>
#include <json.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define DEBUG_DEFRAG_KVS 0
#define DEBUG_DEFRAG_ARRAY_ITEMS 0

void *yy_scan_buffer  (char * base, size_t  size );
void *yy_scan_bytes ( const char *bytes, int len  );
void  yy_delete_buffer (void *);
void yy_set_parser(json_parser_t *p);
void yy_set_json(json_t *j);
int  yylex_destroy(void);
extern int yyparse(void);
extern int line;

json_value_t *json_find_value(json_t *j, void *o);
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

void json_print_debug_array_items(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_array_items; i++) {
    printf("item [%zu] { parent=%p", i, j->array_items[i].parent);
    if (j->array_items[i].prev != NULL) {
      printf(", prev=%zu", j->array_items[i].prev - j->array_items);
    }
    if (j->array_items[i].next != NULL) {
      printf(", next=%zu", j->array_items[i].next - j->array_items);
    }
    printf(" }\n");
  }
}

void json_print_debug_kvs(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_kvs; i++) {
    printf("kv [%zu] { parent=%p", i, j->kvs[i].parent);
    if (j->kvs[i].prev != NULL) {
      printf(", prev=%zu", j->kvs[i].prev - j->kvs);
    }
    if (j->kvs[i].next != NULL) {
      printf(", next=%zu", j->kvs[i].next - j->kvs);
    }
    printf(" }\n");
  }
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
  json_print_debug_kvs(j);
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
  json_print_debug_array_items(j);
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
  for (i=0; i<j->args.n_array_items; i++) {
    if (v == j->array_items[i].value) {
      json_array_t *a = j->array_items[i].parent;
      json_array_item_t *item = a->first_item;
      json_print_key_val(j, json_find_value(j, a));
      size_t ix = 0;
      while (item != &j->array_items[i]) {
        item = item->next;
	ix++;
      }

      printf("[%zu]", ix);
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

json_value_t *json_find_value(json_t *j, void *o)
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

void json_swap_array_items(json_t *j, size_t i1, size_t i2)
{
  json_array_item_t item_tmp;
  json_array_item_t *item1;
  json_array_item_t *item2;

  if (i1 == i2) { return; }
  item1 = &j->array_items[i1];
  item2 = &j->array_items[i2];

  if (item1->prev == item2) { item1->prev = item1; }
  if (item1->next == item2) { item1->next = item1; }
  if (item2->prev == item1) { item2->prev = item2; }
  if (item2->next == item1) { item2->next = item2; }

  memmove(&item_tmp, item1, sizeof(item_tmp));
  memmove(item1, item2, sizeof(item_tmp));
  memmove(item2, &item_tmp, sizeof(item_tmp));

  if (item1->prev != NULL) { item1->prev->next = item1; }
  if (item1->next != NULL) { item1->next->prev = item1; }
  if (item2->prev != NULL) { item2->prev->next = item2; }
  if (item2->next != NULL) { item2->next->prev = item2; }
}

void json_swap_kvs(json_t *j, size_t i1, size_t i2)
{
  json_kv_t kv_tmp;
  json_kv_t *kv1;
  json_kv_t *kv2;

  if (i1 == i2) { return; }
  kv1 = &j->kvs[i1];
  kv2 = &j->kvs[i2];

  if (kv1->prev == kv2) { kv1->prev = kv1; }
  if (kv1->next == kv2) { kv1->next = kv1; }
  if (kv2->prev == kv1) { kv2->prev = kv2; }
  if (kv2->next == kv1) { kv2->next = kv2; }

  memmove(&kv_tmp, kv1, sizeof(kv_tmp));
  memmove(kv1, kv2, sizeof(kv_tmp));
  memmove(kv2, &kv_tmp, sizeof(kv_tmp));

  if (kv1->prev != NULL) { kv1->prev->next = kv1; }
  if (kv1->next != NULL) { kv1->next->prev = kv1; }
  if (kv2->prev != NULL) { kv2->prev->next = kv2; }
  if (kv2->next != NULL) { kv2->next->prev = kv2; }
}

void json_defrag_array_items(json_t *j)
{
  size_t i;
  size_t defrag_ix = 0;
#if DEBUG_DEFRAG_ARRAY_ITEMS
  printf("PRE defrag array items\n");
  json_print_debug_array_items(j);
#endif
  for (i=0; i<j->args.n_array_items; i++) {
    json_array_item_t *item = &j->array_items[i];
    if (item->prev == NULL) {
      item->parent->first_item = &j->array_items[defrag_ix];
      while (item != NULL) {
        size_t ix = item - j->array_items;
	assert(defrag_ix < j->args.n_array_items);
	json_swap_array_items(j, ix, defrag_ix);
	item = &j->array_items[defrag_ix];
	defrag_ix++;
        item = item->next;
      }
    }
  }
#if DEBUG_DEFRAG_ARRAY_ITEMS
  printf("POST defrag array items\n");
  json_print_debug_array_items(j);
#endif
}

void json_defrag_kvs(json_t *j)
{
  size_t i;
  size_t defrag_ix = 0;
#if DEBUG_DEFRAG_KVS
  printf("PRE defrag\n");
  json_print_debug_kvs(j);
#endif
  for (i=0; i<j->args.n_kvs; i++) {
    json_kv_t *kv = &j->kvs[i];
    if (kv->prev == NULL) {
      kv->parent->first_kv = &j->kvs[defrag_ix];
      while (kv != NULL) {
        size_t ix = kv - j->kvs;
	assert(defrag_ix < j->args.n_kvs);
	json_swap_kvs(j, ix, defrag_ix);
	kv = &j->kvs[defrag_ix];
	defrag_ix++;
        kv = kv->next;
      }
    }
  }
#if DEBUG_DEFRAG_KVS
  printf("POST defrag\n");
  json_print_debug_kvs(j);
#endif
}

void json_defrag(json_t *j)
{
  if (j == NULL) { return; }
  json_defrag_kvs(j);
  json_defrag_array_items(j);
}

json_object_t *json_root_object(json_t *j)
{
  if (j == NULL) { return NULL; }
  return &j->objects[j->args.n_objects - 1];
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

