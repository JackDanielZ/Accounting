#include <Eina.h>

#include "common.h"

static Month_Item *
_month_item_find(Month_History *hist, Item_Desc *idesc)
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

static Item_Desc *
_item_desc_find_rec(Item_Desc *cur_desc, Eina_Stringshare *categ, int depth)
{
   Eina_List *itr;
   Eina_Stringshare *nick;
   Item_Desc *sub_desc;
   if (!cur_desc) return NULL;
   EINA_LIST_FOREACH(cur_desc->nicknames, itr, nick)
     {
        if (nick == categ) return cur_desc;
     }
   if (!depth) return NULL;
   EINA_LIST_FOREACH(cur_desc->subitems, itr, sub_desc)
     {
        Item_Desc *sub_ret = _item_desc_find_rec(sub_desc, categ, depth - 1);
        if (sub_ret) return sub_ret;
     }
   return NULL;
}

static Item_Desc *
_item_desc_find(Year_Desc *ydesc, Item_Desc *pdesc, Eina_Stringshare *categ, Eina_Bool depth)
{
   if (pdesc) return _item_desc_find_rec(pdesc, categ, depth);
   else
     {
        Eina_List *itr;
        Item_Desc *sub_ret;
        EINA_LIST_FOREACH(ydesc->debits, itr, pdesc)
          {
             sub_ret = _item_desc_find_rec(pdesc, categ, depth);
             if (sub_ret) return sub_ret;
          }
        EINA_LIST_FOREACH(ydesc->credits, itr, pdesc)
          {
             sub_ret = _item_desc_find_rec(pdesc, categ, depth);
             if (sub_ret) return sub_ret;
          }
        EINA_LIST_FOREACH(ydesc->savings, itr, pdesc)
          {
             sub_ret = _item_desc_find_rec(pdesc, categ, depth);
             if (sub_ret) return sub_ret;
          }
     }
   return NULL;
}

static Item_Desc *
_other_item_find(Year_Desc *ydesc, Item_Desc *pdesc)
{
   Eina_List *itr;
   EINA_LIST_FOREACH(pdesc ? pdesc->subitems : ydesc->debits, itr, pdesc)
     {
        if (pdesc->as_other) return pdesc;
     }
   return NULL;
}

static Eina_Bool
_chunk_handle(char *chunk, Year_Desc *ydesc, Month_History *hist)
{
   /*
    * reverse look up float number
    * remains the operation name
    * reverse extract destination (should be at the end) @... if exists
    * stringshare destination and operation and look into items names
    */
   Month_Item *parent_mitem = NULL;
   Eina_Bool is_minus = EINA_FALSE;
   trailing_remove(chunk);
   float sum = -1.0;
   char *ptr = strrchr(chunk, ' '); /* Look for sum */
   if (!ptr) return EINA_FALSE;
   sum = atof(ptr + 1);
   *ptr = '\0';

   /* Search for category */
   if ((ptr = strchr(chunk, '@')))
     {
        char *end_categ = ptr + 1;
        while (*end_categ &&
              ((*end_categ >= 'a' && *end_categ <= 'z') ||
               (*end_categ >= 'A' && *end_categ <= 'Z') ||
               (*end_categ >= '0' && *end_categ <= '9')))
          {
             end_categ++;
          }
        my_to_lower(ptr + 1, end_categ - (ptr + 1));
        Eina_Stringshare *category = eina_stringshare_add_length(ptr + 1, end_categ - (ptr + 1));
        Item_Desc *categ_desc = _item_desc_find(ydesc, NULL, category, -1);
        eina_stringshare_del(category);
        if (!categ_desc) return EINA_FALSE;
        parent_mitem = _month_item_find(hist, categ_desc);
        if (*end_categ == '.')
          {
             end_categ++;
             if (!strcmp(end_categ, "max"))
               {
                  end_categ += 3;
                  if (parent_mitem->max)
                    {
                       // FIXME ERROR max already set
                       return EINA_FALSE;
                    }
                  parent_mitem->max = sum;
               }
             else if (!strcmp(end_categ, "expected"))
               {
                  end_categ += 8;
                  if (parent_mitem->expected)
                    {
                       // FIXME ERROR expected already set
                       return EINA_FALSE;
                    }
                  parent_mitem->expected = sum;
               }
             else
               {
                  return EINA_FALSE;
               }
             return EINA_TRUE;
          }
        else if (*end_categ == '+' || *end_categ == '-')
          {
             is_minus = (*end_categ == '-');
          }
        if (ptr == chunk) chunk = end_categ;
        else *ptr = '\0';
     }
   trailing_remove(chunk);
   char *lower_chunk = strdup(chunk);
   my_to_lower(lower_chunk, -1);
   Eina_Stringshare *chunk_shr = eina_stringshare_add(chunk);
   Eina_Stringshare *lchunk_shr = eina_stringshare_add(lower_chunk);
   free(lower_chunk);
   Item_Desc *idesc = _item_desc_find(ydesc, parent_mitem ? parent_mitem->desc : NULL, lchunk_shr, 1);
   eina_stringshare_del(lchunk_shr);
   if (!idesc)
     {
        /* Is there an item for others */
        idesc = _other_item_find(ydesc, parent_mitem ? parent_mitem->desc : NULL);
     }
   if (!idesc) idesc = parent_mitem ? parent_mitem->desc : NULL;
   if (!idesc)
     {
        /* Unknown category*/
        return EINA_FALSE;
     }
   Month_Item *item = _month_item_find(hist, idesc);
   if (item)
     {
        Month_Operation *op = calloc(1, sizeof(*op));
        op->v = sum;
        if (idesc->name != chunk_shr) op->name = eina_stringshare_ref(chunk_shr);
        op->is_minus = is_minus;
        item->ops = eina_list_append(item->ops, op);
     }
   eina_stringshare_del(chunk_shr);
   return !!item;
}

Month_History *
history_parse(const char *buffer, Year_Desc *ydesc)
{
   /*
    * if token == (
    *   extract chunk until & or )
    * else extract chunk until \0 or \n
    * for each chunk
    */
   Eina_Bool success = EINA_FALSE, multi;
   Lexer l;
   l.buffer = buffer;
   lexer_reset(&l);

   Month_History *hist = calloc(1, sizeof(*hist));

   while (1)
     {
        multi = is_next_token(&l, "(");
        if (multi)
          {
             do
               {
                  char *chunk = chunk_get(&l, EINA_FALSE, '&', ')', '\0');
                  if (chunk)
                    {
                       // FIXME error if (not)
                       _chunk_handle(chunk, ydesc, hist);
                    }
                  else goto end;
               }
             while (is_next_token(&l, "&"));
             if (is_next_token(&l, ")"))
               {
                  next_number(&l);
                  // FIXME: consume number
               }
          }
        else
          {
             char *chunk = chunk_get(&l, EINA_FALSE, '\n', '\0');
             if (chunk)
               {
                  // FIXME error if (not)
                  _chunk_handle(chunk, ydesc, hist);
               }
             else goto end;
          }
     }

end:
   success = EINA_TRUE;
error:
   if (!success)
     {
        free(hist);
        // FIXME intern frees
        return NULL;
     }
   return hist;
}

