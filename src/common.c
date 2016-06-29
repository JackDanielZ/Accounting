#include "common.h"

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

int
next_number(Lexer *l)
{
   ws_skip(l);
   const char *str = l->current;
   while (*str && (*str >= '0' && *str <= '9')) str++;
   if (str == l->current) return -1;
   int size = str - l->current;
   char *n_str = alloca(size + 1);
   memcpy(n_str, l->current, size);
   n_str[size] = '\0';
   l->current = str;
   l->offset += size;
   return atoi(n_str);
}

void
error_print(Lexer *l, const char *error_str)
{
   fprintf(stderr, "Parsing error line %d character %d: %s\n",
         l->line_no + 1, l->offset + 1, error_str);
}


