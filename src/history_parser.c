#include <Eina.h>

#include "common.h"

static Eina_Bool
_is_desc_item_named(Item_Desc *idesc, Eina_Stringshare *name_shr)
{
   Eina_List *itr;
   Eina_Stringshare *nick;
   if (!idesc) return EINA_FALSE;
   EINA_LIST_FOREACH(idesc->nicknames, itr, nick)
     {
        if (nick == name_shr) return EINA_TRUE;
     }
   return EINA_FALSE;
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
        Item_Desc *sub_ret = _item_desc_find(ydesc, ydesc->debits, categ, depth);
        if (sub_ret) return sub_ret;
        sub_ret = _item_desc_find(ydesc, ydesc->credits, categ, depth);
        if (sub_ret) return sub_ret;
        sub_ret = _item_desc_find(ydesc, ydesc->savings, categ, depth);
        if (sub_ret) return sub_ret;
     }
   return NULL;
}

static Item_Desc *
_other_item_find(Year_Desc *ydesc, Item_Desc *pdesc)
{
   Eina_List *itr;
   if (!pdesc) pdesc = ydesc->debits;
   if (!pdesc) return NULL;
   EINA_LIST_FOREACH(pdesc->subitems, itr, pdesc)
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
   Eina_Stringshare *chunk_shr = NULL;
   Eina_Stringshare *lchunk_shr = NULL;
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
        if (!categ_desc)
          {
             fprintf(stderr, "Category %s not found\n", category);
             eina_stringshare_del(category);
             return EINA_FALSE;
          }
        eina_stringshare_del(category);
        parent_mitem = month_item_find(hist, categ_desc);
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
   chunk_shr = eina_stringshare_add(chunk);
   lchunk_shr = eina_stringshare_add(lower_chunk);
   free(lower_chunk);
   Item_Desc *idesc = _item_desc_find(ydesc, parent_mitem ? parent_mitem->desc : NULL, lchunk_shr, -1);
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
   if (idesc->as_trash) return EINA_TRUE;
   Month_Item *item = month_item_find(hist, idesc);
   if (item)
     {
        Month_Operation *op = calloc(1, sizeof(*op));
        op->v = sum;
        if (!_is_desc_item_named(idesc, lchunk_shr))
           op->name = eina_stringshare_ref(chunk_shr);
        op->is_minus = is_minus;
        item->ops = eina_list_append(item->ops, op);
     }
   eina_stringshare_del(chunk_shr);
   eina_stringshare_del(lchunk_shr);
   return !!item;
}

Eina_Bool
history_parse(const char *buffer, int month, Year_Desc *ydesc)
{
   /*
    * if token == (
    *   extract chunk until & or )
    * else extract chunk until \0 or \n
    * for each chunk
    */
   Eina_Bool success = EINA_FALSE;
   Lexer l;
   l.buffer = buffer;
   lexer_reset(&l);

   Month_History *hist = calloc(1, sizeof(*hist));
   hist->month = month;

   do
     {
        char *chunk = chunk_get(&l, EINA_FALSE, '\n', '&', '=', '\0');
        if (chunk)
          {
             if (!_chunk_handle(chunk, ydesc, hist))
               {
                  ERROR_PRINT(&l, "Chunk error");
                  fprintf(stderr, "%s\n", chunk);
                  return EINA_FALSE;
               }
          }
        else goto end;
        if (is_next_token(&l, "="))
          {
             next_number(&l);
          }
        else
          {
             is_next_token(&l, "\n");
             is_next_token(&l, "&");
          }
     }
   while (1);

end:
   success = EINA_TRUE;
error:
   if (!success)
     {
        free(hist);
        // FIXME intern frees
        return EINA_FALSE;
     }
   ydesc->months = eina_list_append(ydesc->months, hist);
   return EINA_TRUE;
}

