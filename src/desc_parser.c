#include <Eina.h>

#include "common.h"

static Eina_List *_items_parse(Lexer *l);

static Item_Desc *
_item_parse(Lexer *l)
{
   Item_Desc *idesc = NULL;
   char *elt = next_word(l, " ", EINA_TRUE);
   Eina_Bool end = !elt;
   while (!end)
     {
        if (!idesc) idesc = calloc(1, sizeof(*idesc));
        if (elt)
          {
             trailing_remove(elt);
             if (!idesc->name) idesc->name = eina_stringshare_add(elt);
             eina_str_tolower(&elt);
             Eina_Stringshare *shr = eina_stringshare_add(elt);
             free(elt);
             idesc->nicknames = eina_list_append(idesc->nicknames, shr);
          }
        if (is_next_token(l, "@or"))
          {
             elt = next_word(l, " ", EINA_TRUE);
          }
        else if (is_next_token(l, "@other"))
          {
             idesc->as_other = EINA_TRUE;
             elt = NULL;
          }
        else if (is_next_token(l, "@trash"))
          {
             idesc->as_trash = EINA_TRUE;
             elt = NULL;
          }
        else
          {
             idesc->subitems = _items_parse(l);
             end = EINA_TRUE;
          }
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
             Item_Desc *idesc = _item_parse(&l);
             if (!strcmp(idesc->name, "Debits")) ydesc->debits = idesc;
             if (!strcmp(idesc->name, "Credits")) ydesc->credits = idesc;
             if (!strcmp(idesc->name, "Savings")) ydesc->savings = idesc;
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
