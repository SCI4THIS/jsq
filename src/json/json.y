%{
#include <stdio.h>
#include <string.h>
#include <json.h>
#include <json_parser.h>
#include <json_int.h>
#define DEBUG 0
json_string_t  yy_json_string = { 0 };
json_string_t *key;
json_int_t    yy_json_int = { 0 };
json_double_t yy_json_double = { 0 };
json_value_t  yy_json_value = { 0 };
json_parser_t *yyparser = NULL;
json_t *yyjson = NULL;

int   yylex();
int   yyerror(const char *msg);
void *yyalloc(size_t bytes);
void  yyfree(void *ptr);
void  yy_set_parser(json_parser_t *p);
void  yy_set_json(json_t *j);
void *yy_handle_string(json_string_t *val);
void *yy_handle_int(json_int_t *val);
void *yy_handle_double(json_double_t *val);
void *yy_handle_value(json_value_type_t t, void *payload);
void *yy_handle_kv(json_string_t *k, json_value_t *val);
void *yy_handle_kv_link(json_kv_t *prev, json_kv_t *next);
void *yy_handle_array(json_array_item_t *last_item);
void *yy_handle_array_item(json_value_t *val);
void *yy_handle_array_item_link(json_array_item_t *prev, json_array_item_t *next);
void *yy_handle_object(json_kv_t *last_value);
void *yy_handle_root_object(json_object_t *o);

#define YYMALLOC yyalloc
#define YYFREE   yyfree
%}
%define api.value.type { void * }

%token T_true
%token T_false
%token T_string
%token T_num_integer
%token T_num_double
%token T_null

%%

start:
  object { $$ = yy_handle_root_object($1); }
;

object:
  '{' '}' { $$ = yy_handle_object(NULL); }
| '{' key_value_pairs '}' { $$ = yy_handle_object($2); }
;

array:
  '[' ']' { $$ = yy_handle_array(NULL); }
| '[' values ']' { $$ = yy_handle_array($2); }

values:
  value { $$ = yy_handle_array_item($1); }
| values ',' value { $$ = yy_handle_array_item_link($1, yy_handle_array_item($3)); }
;

key_value_pairs:
   key_value_pair { $$ = $1; }
|  key_value_pairs ',' key_value_pair { $$ = yy_handle_kv_link($1, $3); }
;

key_value_pair:
  key ':' value { $$ = yy_handle_kv($1, $3); }
;

key:
  string { $$ = $1; }
;

string:
  T_string { $$ = yy_handle_string($1); }
;

value:
  T_true  { $$ = yy_handle_value(JSON_VALUE_TYPE_TRUE, NULL); }
| T_false { $$ = yy_handle_value(JSON_VALUE_TYPE_FALSE, NULL); }
| string  { $$ = yy_handle_value(JSON_VALUE_TYPE_STRING, $1); }
| object  { $$ = yy_handle_value(JSON_VALUE_TYPE_OBJECT, $1); }
| array   { $$ = yy_handle_value(JSON_VALUE_TYPE_ARRAY, $1); }
| T_num_integer { $$ = yy_handle_value(JSON_VALUE_TYPE_INT, yy_handle_int($1)); }
| T_num_double { $$ = yy_handle_value(JSON_VALUE_TYPE_DOUBLE, yy_handle_double($1)); }
| T_null    { $$ = yy_handle_value(JSON_VALUE_TYPE_NULL, NULL); }
;

%%

void *yy_handle_root_object(json_object_t *o)
{
  size_t i;
  json_defrag(yyjson);
}

void* yy_handle_object(json_kv_t *last_kv)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_objects;
  void *rc = NULL;
  p->args.n_objects++;
  if (j != NULL) {
    json_object_t *o = &j->objects[i];
    if (last_kv == NULL) {
      o->n = 0;
      o->first_kv = NULL;
    } else {
      json_kv_t *kv = last_kv;
      size_t n = 0;
      json_kv_t *kv_ = kv;
      while (kv != NULL) {
        kv->parent = o;
        kv_ = kv;
        kv = kv->prev;
        n++;
      }
      o->n = n;
      o->first_kv = kv_;
    }
    rc = o;
  }
  return rc;
}

void* yy_handle_array(json_array_item_t *last_item)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_arrays;
  void *rc = NULL;
  p->args.n_arrays++;
  if (j != NULL) {
    json_array_t *a = &j->arrays[i];
    if (last_item == NULL) {
      a->n = 0;
      a->first_item = NULL;
    } else {
      size_t n = 0;
      json_array_item_t *item = last_item;
      json_array_item_t *item_ = item;
      while (item != NULL) {
        item_ = item;
        item->parent = a;
        item = item->prev;
        n++;
      }
      a->n = n;
      a->first_item = item_;
    }
    rc = a;
  }
  return rc;
}

void *yy_handle_value(json_value_type_t t, void *payload)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_values;
  void *rc = NULL;
  p->args.n_values++;
  if (j != NULL) {
    j->values[i].type = t;
    j->values[i].payload = payload;
    rc = &j->values[i];
  }
  return rc;
}

void* yy_handle_string(json_string_t *val)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_strings;
  void *rc = NULL;
  p->args.n_strings++;
  if (j != NULL) {
    char *stab_s = &j->stab[p->args.n_stab];
    rc = &j->strings[i];
    memmove(stab_s, val->s, val->len);
    val->s = stab_s;
    memmove(rc, val, sizeof(json_string_t));
  }
  p->args.n_stab += val->len;
  return rc;
}

void* yy_handle_int(json_int_t *val)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_ints;
  void *rc = NULL;
  p->args.n_ints++;
  if (j != NULL) {
    rc = &j->ints[i];
    memmove(rc, val, sizeof(json_int_t));
  }
  return rc;
}

void *yy_handle_double(json_double_t *val)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_doubles;
  void *rc = NULL;
  p->args.n_doubles++;
  if (j != NULL) {
    rc = &j->doubles[i];
    memmove(rc, val, sizeof(json_double_t));
  }
  return rc;
}

void *yy_handle_kv_link(json_kv_t *prev, json_kv_t *next)
{
  if (prev != NULL && next != NULL) {
    prev->next = next;
    next->prev = prev;
    return next;
  }
  return NULL;
}

void *yy_handle_array_item(json_value_t *val)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_array_items;
  void *rc = NULL;
  p->args.n_array_items++;
  if (j != NULL) {
    j->array_items[i].value = val;
    j->array_items[i].next = NULL;
    j->array_items[i].prev = NULL;
    rc = &j->array_items[i];
  }
  return rc;
}

void *yy_handle_array_item_link(json_array_item_t *prev, json_array_item_t *next)
{
  if (prev != NULL && next != NULL) {
    prev->next = next;
    next->prev = prev;
    return next;
  }
  return NULL;
}

void *yy_handle_kv(json_string_t *k, json_value_t *val)
{
  json_parser_t *p = yyparser;
  json_t *j = yyjson;
  size_t i = p->args.n_kvs;
  void *rc = NULL;
  p->args.n_kvs++;
  if (j != NULL) {
    j->kvs[i].key = k;
    j->kvs[i].value = val;
    j->kvs[i].next = NULL;
    rc = &j->kvs[i];
  }
  return rc;
}

size_t yy_compute_offset(const uint8_t *s)
{
  return s - yyparser->buf;
}

void yy_set_parser(json_parser_t *p)
{
  yyparser = p;
}

void yy_set_json(json_t *j)
{
#if DEBUG
  printf("yy_set_json(%p)\n", j);
#endif
  yyjson = j;
}
