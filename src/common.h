#ifndef _COMMON_H
#define _COMMON_H

#include <Eina.h>

typedef struct
{
   const char *buffer;
   const char *current;
   unsigned int line_no;
   unsigned int offset;
} Lexer;

void lexer_reset(Lexer *l);

void ws_skip(Lexer *l);

Eina_Bool is_next_token(Lexer *l, const char *token);

char *next_word(Lexer *l, const char *special, Eina_Bool special_allowed);

int next_number(Lexer *l);

void error_print(Lexer *l, const char *error_str);

#define ERROR_PRINT(l, s) \
{ \
   error_print(l, s); \
   return NULL; \
}

#endif

