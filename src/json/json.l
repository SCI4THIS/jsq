%{
#define DEBUG 1
#include <json.h>
#include <json_parser.h>
#include "bison.out.h"
extern json_parser_t *yyparser;
extern json_string_t yy_json_string;
extern json_int_t    yy_json_int;
extern json_double_t yy_json_double;
int line = 1;
int yyerror(const char *msg)
{
    fprintf(stderr, "ERROR: %d: %s\n", line, msg);
}
static char errbuf[128];
int  handle_exponent(const char *s, int len);
int  handle_double(const char *s, int len);
int  handle_integer(const char *s, int len);
int  handle_true(const char *s, int len);
int  handle_false(const char *s, int len);
int  handle_null(const char *s, int len);
int  handle_string(const char *s, int len);
void handle_error(const char *s);
size_t yy_compute_offset(const uint8_t *s);
%}

%option noyyalloc noyyrealloc noyyfree

%%

[-+]?[0-9]*[.]?[0-9]+[Ee][-+]?[0-9]+ {
                        return handle_exponent(yytext, yyleng); }
[-+]?[0-9]*[.][0-9]+  { return handle_double(yytext, yyleng); }
[-+]?[0-9]+           { return handle_integer(yytext, yyleng); }
["]([^"]|[\\]["])*["] { return handle_string(yytext, yyleng); }
true                  { return handle_true(yytext, yyleng); }
false                 { return handle_false(yytext, yyleng); }
null                  { return handle_null(yytext, yyleng); }
[:]                   { return ':'; }
[{]                   { return '{'; }
[}]                   { return '}'; }
[[]                   { return '['; }
[\]]                  { return ']'; }
[,]                   { return ','; }
[ \t\r]+              { }
[\n]                  { line++; }
.                     { handle_error(yytext); }

%%

double sntod(const char *s, int len)
{
/*

requires a string matching this regex:

[-+]?[0-9]*[.][0-9]+

*/
  double val = 0.0;
  double pos = 0.1;
  double sign = 1.0;
  int i = 0;
  int mode = 0;

  if (s[i] == '+') {
    i++;
  } else if (s[i] == '-') {
    i++;
    sign = -1.0;
  }

  for (i; i<len; i++) {
    double d = s[i] - '0';
    if (s[i] == '.') {
      mode = 1;
      continue;
    }
    switch (mode) {
      case 0:
        val = (val * 10.0) + d;
        break;
      case 1:
        val = val + d * pos;
        pos = pos * 0.1;
        break;
    }
  }
  return sign * val;
}

int sntoi(const char *s, int len)
{
/*

requires a string matching this regex:

[-+]?[0-9]+

*/
  int val = 0;
  int i = 0;
  int sign = 1;
  if (s[i] == '+') {
    i++;
  } else if (s[i] == '-') {
    sign = -1;
    i++;
  }
  for (; i<len; i++) {
    val = val * 10 + (s[i] - '0');
  }
  return sign * val;
}

double sntoe(const char *s, int len)
{
/*

requires a string matching this regex:

[-+]?[0-9]*[.]?[0-9]+[Ee][-+]?[0-9]+

*/
  int i;
  double coefficient = 0.0;
  double scalar = 1.0;
  unsigned int exponent;
  int exponent_sign = 1;
  double accum = 10.0;

  for (i=0; i<len; i++) {
    if (s[i] == 'e' || s[i] == 'E') {
      break;
    }
  }
  coefficient = sntod(s, i);
  i++;
  if (s[i] == '+') {
    i++;
  } else if (s[i] == '-') {
    accum = 0.1;
    i++;
  }
  exponent = sntoi(&s[i], len - i);
  for(int j=0; j<8*sizeof(exponent); j++) {
    unsigned int pos = 1 << j;
    if (exponent & pos) {
      scalar = scalar * accum;
      exponent = exponent & ~pos;
      if (exponent == 0) { break; }
    }
    accum = accum * accum;
  }
  return coefficient * scalar;
}

int handle_integer(const char *s, int len)
{
  int val = sntoi(s, len);
  yy_json_int.n = val;
  yylval = &yy_json_int;
  return T_num_integer;
}

int handle_double(const char *s, int len)
{
  double val = sntod(s, len);
  yy_json_double.n = val;
  yylval = &yy_json_double;
  return T_num_double;
}

int handle_exponent(const char *s, int len)
{
  double val = sntoe(s, len);
  yy_json_double.n = val;
  yylval = &yy_json_double;
  return T_num_double;
}

int handle_true(const char *s, int len)
{
  return T_true;
}

int handle_false(const char *s, int len)
{
  return T_false;
}

int handle_null(const char *s, int len)
{
  return T_null;
}

int handle_string(const char *s, int len)
{
  yy_json_string.off = yy_compute_offset(s);
  yy_json_string.len = len;
  yylval = &yy_json_string;
  return T_string;
}

void handle_error(const char *s)
{
  snprintf(errbuf, sizeof(errbuf), "Unexpected char: %c", *yytext);
  yyerror(errbuf);
}

void *yyalloc(size_t bytes)
{
  void *m = json_parser_alloc(yyparser, bytes);
#if DEBUG
  printf("yyalloc(%zu) = %p\n", bytes, m);
#endif
  return m;
}

void *yyrealloc(void *ptr, size_t bytes)
{
#if DEBUG
  printf("yyrealloc(%p, %zu)\n", ptr, bytes);
#endif
  return ptr;
}

void yyfree(void *ptr)
{
#if DEBUG
  printf("yyfree(%p)\n", ptr);
#endif
  return;
}
