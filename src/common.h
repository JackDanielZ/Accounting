#ifndef _COMMON_H
#define _COMMON_H

#include <Eina.h>

typedef struct
{
   Eina_Stringshare *name;
   Eina_List *nicknames; /* List of Eina_Stringshare */
   Eina_List *subitems;
   Eina_Bool as_other : 1;
} Item_Desc;

typedef struct
{
   Eina_List *debits;
   Eina_List *credits;
   Eina_List *savings;
   int year;
} Year_Desc;

typedef struct
{
   Eina_Stringshare *name; /* Only for other */
   float v;
   Eina_Bool is_minus : 1;
} Month_Operation;

typedef struct
{
   Item_Desc *desc;
   Eina_List *ops;
   float max;
   float expected;
} Month_Item;

typedef struct
{
   Eina_List *items;
} Month_History;

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

char *chunk_get(Lexer *l, Eina_Bool include, char token, ...);

char *line_get(Lexer *l);

void error_print(Lexer *l, const char *error_str);

void trailing_remove(char *str);

void my_to_lower(char *ptr, int len);

#define ERROR_PRINT(l, s) \
{ \
   error_print(l, s); \
   return NULL; \
}

Year_Desc *desc_parse(const char *buffer);

Month_History *history_parse(const char *buffer, Year_Desc *ydesc);

#endif

