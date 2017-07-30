#include "common.h"

static List *_items_parse(Lexer *l, Item_Desc *parent);

static Item_Desc *
_item_parse(Lexer *l, Item_Desc *parent)
{
   Item_Desc *idesc = NULL;
   char *elt = next_word(l, " ", 1);
   int end = !elt;
   while (!end)
     {
        if (!idesc)
          {
             idesc = calloc(1, sizeof(*idesc));
             idesc->parent = parent;
          }
        if (elt)
          {
             char *p;
             trailing_remove(elt);
             if (!idesc->name) idesc->name = strdup(elt);
             for (p = elt; (*p); p++) *p = tolower((unsigned char )(*p));
             char *shr = strdup(elt);
             free(elt);
             idesc->nicknames = list_append(idesc->nicknames, shr);
          }
        if (is_next_token(l, "@or"))
          {
             elt = next_word(l, " ", 1);
          }
        else if (is_next_token(l, "@other"))
          {
             idesc->as_other = 1;
             elt = NULL;
          }
        else if (is_next_token(l, "@trash"))
          {
             idesc->as_trash = 1;
             elt = NULL;
          }
        else if (is_next_token(l, "@"))
          {
             char *individual = next_word(l, "", 1), *p;
             for (p = individual; (*p); p++) *p = tolower((unsigned char )(*p));
             idesc->individual = strdup(individual);
             end = 1;
             free(individual);
          }
        else
          {
             idesc->subitems = _items_parse(l, idesc);
             end = 1;
          }
     }
   return idesc;
}

static List *
_items_parse(Lexer *l, Item_Desc *parent)
{
   List *lst = NULL;
   if (is_next_token(l, "{"))
     {
        while (!is_next_token(l, "}"))
          {
             Item_Desc *idesc = _item_parse(l, parent);
             if (idesc) lst = list_append(lst, idesc);
          }
     }
   return lst;
}

Year_Desc *
desc_parse(const char *buffer)
{
   Item_Desc *idesc;
   List *itr;
   int success = 0;
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
             if (is_next_token(&l, "@inherit_remainings")) ydesc->inherit_remainings = 1;
             else
               {
                  idesc = _item_parse(&l, NULL);
                  if (!strcmp(idesc->name, "Individuals")) ydesc->individuals = idesc;
                  if (!strcmp(idesc->name, "Debits")) ydesc->debits = idesc;
                  if (!strcmp(idesc->name, "Credits")) ydesc->credits = idesc;
                  if (!strcmp(idesc->name, "Savings")) ydesc->savings = idesc;
               }
          }
     }
   if (ydesc->savings)
      LIST_FOREACH(ydesc->savings->subitems, itr, idesc)
         idesc->as_saving = 1;

   success = 1;
end:
   if (!success)
     {
        free(ydesc);
        return NULL;
     }
   return ydesc;
}
