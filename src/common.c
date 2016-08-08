#include "common.h"

#include <ctype.h>

void
lexer_reset(Lexer *l)
{
   l->current = l->buffer;
   l->line_no = l->offset = 0;
}

void
ws_skip(Lexer *l)
{
   /*
    * Skip spaces and \n
    * For \n, inc line_no and reset offset
    * otherwise inc offset
    */
   do
     {
        char c = *(l->current);
        switch (c)
          {
           case ' ':
              l->offset++;
              break;
           case '\n':
              l->line_no++;
              l->offset = 0;
              break;
           default:
              return;
          }
        l->current++;
     }
   while (1);
}

Eina_Bool
is_next_token(Lexer *l, const char *token)
{
   ws_skip(l);
   if (!strncmp(l->current, token, strlen(token)))
     {
        l->current += strlen(token);
        l->offset += strlen(token);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

char *
next_word(Lexer *l, const char *special, Eina_Bool special_allowed)
{
   if (!special) special = "";
   ws_skip(l);
   const char *str = l->current;
   while (*str &&
         ((*str >= 'a' && *str <= 'z') ||
          (*str >= 'A' && *str <= 'Z') ||
          (*str >= '0' && *str <= '9') ||
          !(!!special_allowed ^ !!strchr(special, *str)) ||
          *str == '_')) str++;
   if (str == l->current) return NULL;
   int size = str - l->current;
   char *word = malloc(size + 1);
   memcpy(word, l->current, size);
   word[size] = '\0';
   l->current = str;
   l->offset += size;
   return word;
}

float
next_number(Lexer *l)
{
   ws_skip(l);
   const char *str = l->current;
   while (*str && ((*str >= '0' && *str <= '9') || *str == '.')) str++;
   if (str == l->current) return -1;
   int size = str - l->current;
   char *n_str = alloca(size + 1);
   memcpy(n_str, l->current, size);
   n_str[size] = '\0';
   l->current = str;
   l->offset += size;
   return atoi(n_str);
}

char *
chunk_get(Lexer *l, Eina_Bool include, char token, ...)
{
   va_list list;
   ws_skip(l);
   va_start(list, token);
   const char *begin = l->current;
   char *min_ptoken = NULL, min_token = '\0';
   min_ptoken = strchr(l->current, token);
   do
     {
        token = va_arg(list, int);
        char *ptr = strchr(l->current, token);
        if (ptr && (!min_ptoken || min_ptoken > ptr)) min_ptoken = ptr;
     }
   while (token);
   if (min_ptoken)
     {
        l->current = min_ptoken + (!!include ? 1 : 0);
        if (min_token == '\n')
          {
             l->offset = 0;
             l->line_no++;
          }
     }
   if (begin == l->current) return NULL;
   int size = l->current - begin;
   char *chunk = malloc(size + 1);
   memcpy(chunk, begin, size);
   chunk[size] = '\0';
   return chunk;
}

void
error_print(Lexer *l, const char *error_str)
{
   fprintf(stderr, "Parsing error line %d character %d: %s\n",
         l->line_no + 1, l->offset + 1, error_str);
}

void
trailing_remove(char *str)
{
   int len = strlen(str);
   while (str[len - 1] == ' ')
     {
        str[len - 1] = '\0';
        len--;
     }
}

void
my_to_lower(char *ptr, int len)
{
   int i;
   if (len == -1) len = strlen(ptr);
   for (i = 0; i < len; i++)
     {
        ptr[i] = tolower(ptr[i]);
     }
}

Month_Item *
month_item_find(Month_History *hist, Item_Desc *idesc)
{
   Month_Item *item;
   Eina_List *itr;
   EINA_LIST_FOREACH(hist->items, itr, item)
     {
        if (item->desc == idesc) return item;
     }
   item = calloc(1, sizeof(*item));
   item->desc = idesc;
   hist->items = eina_list_append(hist->items, item);
   return item;
}

