#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "list.h"

#define ERR(fmt, ...) fprintf(stderr, fmt"\n", ## __VA_ARGS__)

typedef enum
{
   CALC_POSITIVE  = 1 << 0,
   CALC_NEGATIVE  = 1 << 1,
   CALC_INIT      = 1 << 2,
   CALC_INDIVIDUALS = 1 << 3,
   CALC_BASIC     = CALC_POSITIVE | CALC_NEGATIVE | CALC_INIT
} Calc_Filtering;

typedef struct _Item_Desc Item_Desc;

struct _Item_Desc
{
   const char *name;
   List *nicknames; /* List of Eina_Stringshare */
   List *subitems;
   Item_Desc *parent;
   const char *individual; /* The individual, if defined, to which this item is assigned */
   int as_other : 1;
   int as_trash : 1;
   int as_saving : 1;
};

typedef struct
{
   int year;
   char *files_dir;
   Item_Desc *individuals;
   Item_Desc *debits;
   Item_Desc *credits;
   Item_Desc *savings;
   List *months;
   int inherit_remainings : 1;

   void *ui_data;
} Year_Desc;

typedef struct
{
   const char *name; /* Only for other */
   const char *comment;
   double v;
   int is_minus : 1;
} Month_Operation;

typedef struct
{
   Item_Desc *desc;
   List *ops;
   double max;
   double expected;
   double init;
} Month_Item;

typedef struct
{
   int month;
   List *items;
   int simulation : 1;
} Month_History;

typedef struct
{
   const char *buffer;
   const char *current;
   unsigned int line_no;
   unsigned int offset;
} Lexer;

char* file_get_as_string(const char *filename);

void lexer_reset(Lexer *l);

void ws_skip(Lexer *l);

int is_next_token(Lexer *l, const char *token);

char *next_word(Lexer *l, const char *special, int special_allowed);

double next_number(Lexer *l);

char *chunk_get(Lexer *l, int include, const char *token, ...);

void error_print(Lexer *l, const char *error_str);

void trailing_remove(char *str);

void my_to_lower(char *ptr, int len);

Month_Item *
month_item_find(Month_History *hist, Item_Desc *idesc);

Month_History *
month_hist_get(Year_Desc *ydesc, int month);

unsigned int
year_months_get(Year_Desc *ydesc);

Item_Desc *
individual_find(Year_Desc *ydesc, const char *name);

int
does_idesc_fit_name(Year_Desc *ydesc, Item_Desc *idesc, const char *name);

double
idesc_sum_calc(Year_Desc *ydesc, Month_History *hist, Item_Desc *idesc, char *tooltip, Calc_Filtering filter, const char *individual, double *expected);

#define ERROR_PRINT(l, s) \
{ \
   error_print(l, s); \
}

Year_Desc *desc_parse(const char *buffer);

int history_parse(const char *buffer, int month, Year_Desc *ydesc);

int pdf_generate(Year_Desc *ydesc, const char *output);

int html_generate(Year_Desc *ydesc, const char *output);

#endif

