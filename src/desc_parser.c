#include <Eina.h>

#include "common.h"

static Eina_List *_items_parse(Lexer *l);

static Item_Desc *
_item_parse(Lexer *l)
{
   Item_Desc *idesc = NULL;
   char *line = next_word(l, " ", EINA_TRUE);
   if (line)
     {
        idesc = calloc(1, sizeof(*idesc));
        idesc->name = eina_stringshare_add(line); /* FIXME: have to support OR */
        idesc->subitems = _items_parse(l);
     }
   return idesc;
}

static Eina_List *
_items_parse(Lexer *l)
{
   Eina_List *lst = NULL;
   if (is_next_token(l, "{"))
     {
        while (!is_next_token(l, "}"))
          {
             Item_Desc *idesc = _item_parse(l);
             if (idesc) lst = eina_list_append(lst, idesc);
          }
     }
   return lst;
}

Year_Desc *
desc_parse(const char *buffer)
{
   Eina_Bool success = EINA_FALSE;
   Lexer l;
   l.buffer = buffer;
   lexer_reset(&l);

   int year = next_number(&l);
   if (year == -1) goto end;
   Year_Desc *ydesc = calloc(1, sizeof(*ydesc));
   ydesc->year = year;
   if (is_next_token(&l, "{"))
     {
        while (!is_next_token(&l, "}"))
          {
             if (is_next_token(&l, "Debits")) ydesc->debits = _items_parse(&l);
             if (is_next_token(&l, "Credits")) ydesc->credits = _items_parse(&l);
             if (is_next_token(&l, "Savings")) ydesc->savings = _items_parse(&l);
          }
     }

   success = EINA_TRUE;
end:
   if (!success)
     {
        free(ydesc);
        return NULL;
     }
   return ydesc;
}
