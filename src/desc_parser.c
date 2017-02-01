#include <Eina.h>

#include "common.h"

static Eina_List *_items_parse(Lexer *l, Item_Desc *parent);

static Item_Desc *
_item_parse(Lexer *l, Item_Desc *parent)
{
   Item_Desc *idesc = NULL;
   char *elt = next_word(l, " ", EINA_TRUE);
   Eina_Bool end = !elt;
   while (!end)
     {
        if (!idesc)
          {
             idesc = calloc(1, sizeof(*idesc));
             idesc->parent = parent;
          }
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
        else if (is_next_token(l, "@"))
          {
             char *individual = next_word(l, "", EINA_TRUE);
             eina_str_tolower(&individual);
             idesc->individual = eina_stringshare_add(individual);
             end = EINA_TRUE;
             free(individual);
          }
        else
          {
             idesc->subitems = _items_parse(l, idesc);
             end = EINA_TRUE;
          }
     }
   return idesc;
}

static Eina_List *
_items_parse(Lexer *l, Item_Desc *parent)
{
   Eina_List *lst = NULL;
   if (is_next_token(l, "{"))
     {
        while (!is_next_token(l, "}"))
          {
             Item_Desc *idesc = _item_parse(l, parent);
             if (idesc) lst = eina_list_append(lst, idesc);
          }
     }
   return lst;
}

Year_Desc *
desc_parse(const char *buffer)
{
   Item_Desc *idesc;
   Eina_List *itr;
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
             idesc = _item_parse(&l, NULL);
             if (!strcmp(idesc->name, "Individuals")) ydesc->individuals = idesc;
             if (!strcmp(idesc->name, "Debits")) ydesc->debits = idesc;
             if (!strcmp(idesc->name, "Credits")) ydesc->credits = idesc;
             if (!strcmp(idesc->name, "Savings")) ydesc->savings = idesc;
          }
     }
   if (ydesc->savings)
      EINA_LIST_FOREACH(ydesc->savings->subitems, itr, idesc)
         idesc->as_saving = EINA_TRUE;

   success = EINA_TRUE;
end:
   if (!success)
     {
        free(ydesc);
        return NULL;
     }
   return ydesc;
}
