#include "common.h"

#include <ctype.h>

const char *_months[] =
{
   "January", "February", "March",
   "April", "May", "June", "July",
   "August", "September", "October",
   "November", "December"
};

char*
file_get_as_string(const char *filename)
{
   char *file_data = NULL;
   int file_size;
   FILE *fp = fopen(filename, "r");
   if (!fp)
     {
        ERR("Can not open file: \"%s\".", filename);
        return NULL;
     }

   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   if (file_size <= 0)
     {
        fclose(fp);
        if (file_size < 0) ERR("Can not ftell file: \"%s\".", filename);
        return NULL;
     }
   rewind(fp);
   file_data = (char *) calloc(1, file_size + 1);
   if (!file_data)
     {
        fclose(fp);
        ERR("Calloc failed");
        return NULL;
     }
   int res = fread(file_data, 1, file_size, fp);
   if (!res)
     {
        free(file_data);
        file_data = NULL;
        if (!feof(fp)) ERR("fread failed");
     }
   fclose(fp);
   return file_data;
}

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
   return atof(n_str);
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
   if (!ptr) return;
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
   if (!hist) return NULL;
   EINA_LIST_FOREACH(hist->items, itr, item)
     {
        if (item->desc == idesc) return item;
     }
   item = calloc(1, sizeof(*item));
   item->desc = idesc;
   hist->items = eina_list_append(hist->items, item);
   return item;
}

Month_History *
month_hist_get(Year_Desc *ydesc, int month)
{
   Eina_List *itr;
   Month_History *hist = NULL;
   EINA_LIST_FOREACH(ydesc->months, itr, hist)
     {
        if (hist->month == month) return hist;
     }
   return hist;
}

Item_Desc *
individual_find(Year_Desc *ydesc, Eina_Stringshare *name)
{
   Eina_List *itr, *itr2, *l = ydesc->individuals ? ydesc->individuals->subitems : NULL;
   Item_Desc *individual;
   if (!name) return NULL;
   EINA_LIST_FOREACH(l, itr, individual)
     {
        Eina_Stringshare *ind_name;
        if (individual->name == name) return individual;
        EINA_LIST_FOREACH(individual->nicknames, itr2, ind_name)
          {
             if (ind_name == name) return individual;
          }
     }
   return NULL;
}

Eina_Bool
does_idesc_fit_name(Year_Desc *ydesc, Item_Desc *idesc, Eina_Stringshare *name)
{
   Eina_List *itr;
   Eina_Stringshare *nick;
   Item_Desc *ind_name = individual_find(ydesc, name);
   EINA_LIST_FOREACH(idesc->nicknames, itr, nick)
     {
        Item_Desc *ind_nick = individual_find(ydesc, nick);
        if (nick == name || (ind_name && ind_name == ind_nick))
          {
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

static Eina_Bool
_does_idesc_fit_individual(Year_Desc *ydesc, Item_Desc *idesc, Eina_Stringshare *name)
{
   Item_Desc *ind_name = individual_find(ydesc, name);
   Eina_Bool ind_nick_found = EINA_FALSE;
   if (idesc->individual)
     {
        if (ind_name == individual_find(ydesc, idesc->individual)) return EINA_TRUE;
     }
   else
     {
        Eina_List *itr;
        Eina_Stringshare *nick;
        EINA_LIST_FOREACH(idesc->nicknames, itr, nick)
          {
             Item_Desc *ind_nick = individual_find(ydesc, nick);
             ind_nick_found |= (!!ind_nick);
             if (nick == name || (ind_name && ind_name == ind_nick))
               {
                  return EINA_TRUE;
               }
          }
     }
   if (!name) return !ind_nick_found;
   return EINA_FALSE;
}

/*
 *
 * Calculation:
 * The filter parameter can be used to select specific operations.
 * If max is set, max - sum is set in the expected variable
 * If expected is set, MAX between its value and the actual sum is used. Nothing
 * set in the expected variable.
 * If filter includes CALC_INDIVIDUALS, sum all and dont check individual
 * parameter. Otherwise, if individual == NULL, only sum common stuff else sum
 * only specific items of individual.
 *
 */
float
idesc_sum_calc(Year_Desc *ydesc, Month_History *hist, Item_Desc *idesc,
      Eina_Strbuf *tooltip, Calc_Filtering filter,
      Eina_Stringshare *individual, float *expected_ret)
{
   Eina_List *itr;
   if (!idesc) return 0.0;
   Month_Item *mitem = month_item_find(hist, idesc);
   float sum = filter & CALC_INIT ? mitem->init : 0;
   float expected = 0;
   Month_Operation *op;
   if ((filter & CALC_INIT) && tooltip && sum)
      eina_strbuf_append_printf(tooltip, "Init %.2f\n", sum);
   if (filter & CALC_INDIVIDUALS ||
         (!individual && !idesc->individual) ||
         (individual && _does_idesc_fit_individual(ydesc, idesc, individual)))
     {
        if (individual) filter |= CALC_INDIVIDUALS; /* So for children we dont check individuality */
        EINA_LIST_FOREACH(mitem->ops, itr, op)
          {
             if (((filter & CALC_NEGATIVE) && op->is_minus) ||
                   ((filter & CALC_POSITIVE) && !op->is_minus))
               {
                  float op_sum = (op->v * (op->is_minus ? -1 : 1));
                  sum += op_sum;
                  if (tooltip)
                    {
                       eina_strbuf_append_printf(tooltip, "%.2f", op_sum);
                       if (op->name) eina_strbuf_append_printf(tooltip, ": %s", op->name);
                       if (op->comment) eina_strbuf_append_printf(tooltip, " (%s)", op->comment);
                       eina_strbuf_append_printf(tooltip, "\n");
                    }
               }
          }
     }
   EINA_LIST_FOREACH(idesc->subitems, itr, idesc)
     {
        if ((filter & CALC_INDIVIDUALS) || _does_idesc_fit_individual(ydesc, idesc, individual))
           sum += idesc_sum_calc(ydesc, hist, idesc, NULL, filter, individual, &expected);
     }
   if (mitem->max)
     {
        if (mitem->max > sum) expected = mitem->max - sum;
     }
   else if (mitem->expected)
     {
        if (mitem->expected > sum) sum = mitem->expected;
     }
   if (expected && tooltip)
      eina_strbuf_append_printf(tooltip, "Expected: %.2f\n", expected);
   if (expected_ret) *expected_ret += expected;
   return sum;
}

