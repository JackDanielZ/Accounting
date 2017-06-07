#ifndef _COMMON_H
#define _COMMON_H

#include <Eina.h>
#include <Eo.h>

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
   Eina_Stringshare *name;
   Eina_List *nicknames; /* List of Eina_Stringshare */
   Eina_List *subitems;
   Item_Desc *parent;
   Eina_Stringshare *individual; /* The individual, if defined, to which this item is assigned */
   Eina_Bool as_other : 1;
   Eina_Bool as_trash : 1;
   Eina_Bool as_saving : 1;
};

typedef struct
{
   int year;
   char *files_dir;
   Item_Desc *individuals;
   Item_Desc *debits;
   Item_Desc *credits;
   Item_Desc *savings;
   Eina_List *months;
   Eina_Bool inherit_remainings : 1;

   void *ui_data;
} Year_Desc;

typedef struct
{
   Eina_Stringshare *name; /* Only for other */
   Eina_Stringshare *comment;
   float v;
   Eina_Bool is_minus : 1;
} Month_Operation;

typedef struct
{
   Item_Desc *desc;
   Eina_List *ops;
   float max;
   float expected;
   float init;
} Month_Item;

typedef struct
{
   int month;
   Eina_List *items;
   Eina_Bool simulation : 1;
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

Eina_Bool is_next_token(Lexer *l, const char *token);

char *next_word(Lexer *l, const char *special, Eina_Bool special_allowed);

float next_number(Lexer *l);

char *chunk_get(Lexer *l, Eina_Bool include, char token, ...);

void error_print(Lexer *l, const char *error_str);

void trailing_remove(char *str);

void my_to_lower(char *ptr, int len);

Month_Item *
month_item_find(Month_History *hist, Item_Desc *idesc);

Month_History *
month_hist_get(Year_Desc *ydesc, int month);

Item_Desc *
individual_find(Year_Desc *ydesc, Eina_Stringshare *name);

Eina_Bool
does_idesc_fit_name(Year_Desc *ydesc, Item_Desc *idesc, Eina_Stringshare *name);

float
idesc_sum_calc(Year_Desc *ydesc, Month_History *hist, Item_Desc *idesc, Eina_Strbuf *tooltip, Calc_Filtering filter,
      Eina_Stringshare *individual, float *expected);

#define ERROR_PRINT(l, s) \
{ \
   error_print(l, s); \
}

Year_Desc *desc_parse(const char *buffer);

Eina_Bool history_parse(const char *buffer, int month, Year_Desc *ydesc);

int pdf_generate(Year_Desc *ydesc, const char *output);

int html_generate(Year_Desc *ydesc, const char *output);

#endif

