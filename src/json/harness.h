#ifndef HARNESS_H__
#define HARNESS_H__ 1

#include <stdio.h>
#include <stddef.h>
#include "json.h"
#include "schema.h"

typedef struct json_schema_harness_st json_schema_harness_t;

size_t json_schema_harness(size_t n, json_t **j, json_schema_args_t *args, json_schema_harness_t *js);
json_schema_t *json_schema_harness_schema(json_schema_harness_t *jsh, size_t i);
void json_schema_harness_print(json_schema_harness_t *jsh);
size_t json_schema_harness_write(json_schema_harness_t *jsh, char *buf, size_t len);
size_t json_schema_harness_read(const char *buf, size_t len, json_schema_harness_t *jsh);
size_t json_schema_harness_classify(json_schema_harness_t *jsh, json_t *j);

#endif
