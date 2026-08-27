#include <stdio.h>
#include <json.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define DEBUG_DEFRAG_KVS 0
#define DEBUG_DEFRAG_ARRAY_ITEMS 0

#include <json_int.h>

void *yy_scan_buffer  (char * base, size_t  size );
void *yy_scan_bytes ( const char *bytes, int len  );
void  yy_delete_buffer (void *);
void yy_set_parser(json_parser_t *p);
void yy_set_json(json_t *j);
int  yylex_destroy(void);
extern int yyparse(void);
extern int line;

//------------------------------------------------------------------------------

void json_args_print(json_args_t *args)
{
  printf("JSON ARGS @ %p\n", args);
  if (args == NULL) { return; }
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

void json_print_string(json_string_t *s)
{
  if (s == NULL) { return; }
  printf("%.*s", s->len, s->s);
}

void json_print_abs_key(json_t *j, json_value_t *v)
{
  size_t i;
  json_array_t *a = NULL;
  json_object_t *o = NULL;
  void *rc = NULL;

  if (j == NULL || v == NULL) { return; }
  if (v->parent_kv) {
    json_print_abs_key(j, v->parent_kv->parent_v);
    printf(".");
    json_print_string(v->parent_kv->key);
  }
  if (v->parent_array_item) {
    json_print_abs_key(j, v->parent_array_item->parent_v);
    printf("[%zu]", v->parent_array_item->ix);
  }
}

static void json_print_array(json_array_t *a)
{
  printf("[ ] # len %zu", a->n);
}

static void json_print_object(json_object_t *o)
{
  printf("{ } # %zu kvs", o->n);
}

static void json_print_int(json_int_t *i)
{
  printf("%d", i->n);
}

static void json_print_double(json_double_t *d)
{
  printf("%lf", d->n);
}

void json_print(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_values; i++) {
    json_value_t *v = &j->values[i];
    json_print_abs_key(j, v);
    printf(": ");
    switch (v->type) {
      case JSON_VALUE_TYPE_INVALID:
        printf("INVALID");
	break;
      case JSON_VALUE_TYPE_OBJECT:
	json_print_object(v->payload);
        break;
      case JSON_VALUE_TYPE_ARRAY:
	json_print_array(v->payload);
	break;
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

//------------------------------------------------------------------------------

static void json_swap_array_items(json_t *j, size_t i1, size_t i2)
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

static void json_swap_kvs(json_t *j, size_t i1, size_t i2)
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

static void json_defrag_array_items(json_t *j)
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

static void json_defrag_kvs(json_t *j)
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

static void json_assign_parents(json_t *j)
{
  size_t i;
  for (i=0; i<j->args.n_values; i++) {
    json_value_t *v = &j->values[i];
    if (v->type == JSON_VALUE_TYPE_OBJECT) {
      json_object_t *o = v->payload;
      json_kv_t *kv = o->first_kv;
      while (kv) {
        json_value_t *sub_v = kv->value;
	sub_v->parent_kv = kv;
	kv->parent_v = v;
        kv = kv->next;
      }
    }
    if (v->type == JSON_VALUE_TYPE_ARRAY) {
      size_t ix = 0;
      json_array_t *a = v->payload;
      json_array_item_t *item = a->first_item;
      while (item) {
        json_value_t *sub_v = item->value;
	sub_v->parent_array_item = item;
	item->ix = ix;
	item->parent_v = v;
        item = item->next;
	ix++;
      }
    }
  }
}

void json_defrag(json_t *j)
{
  if (j == NULL) { return; }
  json_defrag_kvs(j);
  json_defrag_array_items(j);
  json_assign_parents(j);
}

//------------------------------------------------------------------------------

bool json_string_eq_s(json_string_t *s1, const char *s2)
{
  size_t len = strlen(s2);
  if (s1 == NULL || s2 == NULL) { return false; }
  if (s1->len == len && memcmp(s1->s, s2, len) == 0) {
    return true;
  }
  return false;
}


json_value_t *json_root(json_t *j)
{
  if (j == NULL) { return NULL; }
  return j->root;
}

json_kv_t *json_kv(json_object_t *o, const char *s, size_t len)
{
  size_t i;
  if (o == NULL) { return NULL; }
  for (i=0; i<o->n; i++) {
    if (len != o->first_kv[i].key->len) {
      continue;
    }
    if (memcmp(o->first_kv[i].key->s, s, len) != 0) {
      continue;
    }
    return &o->first_kv[i];
  }
  return NULL;
}

json_kv_t *json_kv_i(json_object_t *o, size_t i)
{
  if (o == NULL) { return NULL; }
  if (i >= o->n) {
    return NULL;
  }
  return &o->first_kv[i];
}

json_kv_t *json_kv_s(json_object_t *o, const char *s)
{
  size_t len;
  if (o == NULL) { return NULL; }
  len = strlen(s);
  return json_kv(o, s, len);
}

json_string_t *json_kv_key(json_kv_t *kv)
{
  if (kv == NULL) { return NULL; }
  return kv->key;
}

json_object_t *json_kv_parent(json_kv_t *kv)
{
  if (kv == NULL) { return NULL; }
  return kv->parent;
}

json_value_t *json_kv_value(json_kv_t *kv)
{
  if (kv == NULL) { return NULL; }
  return kv->value;
}

size_t json_array_n(json_array_t *a)
{
  if (a == NULL) {
    return 0;
  }
  return a->n;
}

json_value_t *json_array(json_array_t *a, size_t i)
{
  if (a == NULL) {
    return NULL;
  }
  if (i >= a->n) {
    return NULL;
  }
  return a->first_item[i].value;
}

size_t json_string_len(json_string_t *s)
{
  if (s == NULL) { return 0; }
  return s->len;
}

const char *json_string_s(json_string_t *s)
{
  if (s == NULL) { return NULL; }
  return s->s;
}

size_t json_n_kvs(json_object_t *o)
{
  if (o == NULL) { return 0; }
  return o->n;
}

json_value_type_t json_value_type(json_value_t *v)
{
  if (v == NULL) { return JSON_VALUE_TYPE_INVALID; }
  return v->type;
}

void *json_value_payload(json_value_t *v)
{
  if (v == NULL) { return NULL; }
  return v->payload;
}

static size_t json_next_op(const char *s, size_t len)
{
  size_t i;
  for (i=0; i<len; i++) {
    if (s[i] == '.' || s[i] == '[') {
      return i;
    }
  }
  return len;
}

json_value_t *json_value(json_t *j, const char *key, size_t key_len)
{
  size_t i = 0;
  size_t i_ = 0;
  size_t len;
  size_t idx = 0;
  json_value_t *v;
  json_object_t *o;
  json_array_t *a;
  json_kv_t *kv;
  char c;

  if (j == NULL) {
    return NULL;
  }

  v = json_root(j);

  i = 0;
  while (i < key_len) {
    c = key[i];
    switch (c) {
      case '.':
        if (json_value_type(v) != JSON_VALUE_TYPE_OBJECT) {
          return NULL;
	}
	i++;
	o = json_value_payload(v);
	len = json_next_op(&key[i], key_len - i);
	kv = json_kv(o, &key[i], len);
	v = json_kv_value(kv);
	i += len;
        break;
      case '[':
	if (json_value_type(v) != JSON_VALUE_TYPE_ARRAY) {
          return NULL;
	}
	a = json_value_payload(v);
	i_ = i + 1;
	while (i_ < key_len && '0' <= key[i_] && key[i_] <= '9') {
          i_++;
	}
	if (key[i_] != ']') {
          return NULL;
	}
	i_++;
	sscanf(&key[i+1], "[%zu]", &idx);
	i = i_;
	if (idx >= json_array_n(a)) {
          return NULL;
	}
	v = json_array(a, idx);
	break;
      default:
	return NULL;
    }
  }

  return v;
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
