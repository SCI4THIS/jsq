#include <json_parser.h>
#include <string.h>

size_t json_parser(const char *s, size_t siz, json_parser_t *p)
{
  size_t mem_n = (siz + 2) + /* yy_scan_bytes buffer */
                 64 + /* alloc yy_buffer_state */
                 256; /* YY_STACK_ALLOC */
  if (p == 0) {
    return sizeof(json_parser_t) + mem_n;
  }
  memset(&p->args, 0, sizeof(p->args));
  p->s = s;
  p->siz = siz;
  p->mem_i = 0;
  p->mem_n = mem_n;
}

void *json_parser_alloc(json_parser_t *p, size_t n)
{
  void *m;
  if (p == NULL) {
    return NULL;
  }
  if (p->mem_i + n > p->mem_n) {
    return NULL;
  }
  m = &p->buf[p->mem_i];
  p->mem_i += n;
  return m;
}

void json_parser_reset(json_parser_t *p)
{
  memset(&p->args, 0, sizeof(p->args));
  p->mem_i = 0;
}

// clear json_args_t clear_bytes
// yy_scan_bytes (yyalloc(bufsiz + 2) + yyalloc(yy_buffer_state))
// yyparse() - yyalloc (YY_STACK_ALLOC)
//
