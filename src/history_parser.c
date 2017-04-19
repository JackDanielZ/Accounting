#include <Eina.h>

#include "common.h"

static Eina_Bool
_is_desc_item_named(Item_Desc *idesc, const char *name)
{
   Eina_List *itr;
   Eina_Stringshare *nick;
   if (!idesc) return EINA_FALSE;
   char *lname = strdup(name);
   my_to_lower(lname, -1);
   Eina_Stringshare *name_shr = eina_stringshare_add(lname);
   Eina_Bool ret = EINA_TRUE;
   EINA_LIST_FOREACH(idesc->nicknames, itr, nick)
     {
        if (nick == name_shr) goto end;
     }
   ret = EINA_FALSE;
end:
   eina_stringshare_del(name_shr);
   free(lname);
   return ret;
}

static Item_Desc *
_item_desc_candidate_guess(Eina_List *normal_candidates, Eina_List *other_candidates)
{
   Eina_List *itr;
   Item_Desc *idesc, *found = NULL;

   if (normal_candidates)
     {
        if (eina_list_count(normal_candidates) == 1) found = eina_list_data_get(normal_candidates);
        EINA_LIST_FOREACH(normal_candidates, itr, idesc)
          {
             if (found) continue;
             if (!idesc->parent || !idesc->parent->parent) found = idesc;
          }
     }

   if (!found && eina_list_count(other_candidates) == 1) found = eina_list_data_get(other_candidates);
   EINA_LIST_FOREACH(other_candidates, itr, idesc)
     {
        if (found) continue;
        if (!idesc->parent || !idesc->parent->parent) found = idesc;
     }
   return found;
}

static void
_list_candidates(Year_Desc *ydesc, Item_Desc *cur_desc,
      Eina_Stringshare *operation, Eina_Stringshare *category,
      Eina_List **normal_list, Eina_List **other_list)
{
   Eina_List *itr;
   Item_Desc *sub_desc;
   if (!cur_desc) return;
   if (!cur_desc->as_other && !cur_desc->as_saving)
     {
        if (operation && does_idesc_fit_name(ydesc, cur_desc, operation) &&
              (!category
               || does_idesc_fit_name(ydesc, cur_desc->parent, category)
               || category == cur_desc->individual))
           *normal_list = eina_list_append(*normal_list, cur_desc);

        if (!operation && category && does_idesc_fit_name(ydesc, cur_desc, category))
           *normal_list = eina_list_append(*normal_list, cur_desc);

        EINA_LIST_FOREACH(cur_desc->subitems, itr, sub_desc)
          {
             _list_candidates(ydesc, sub_desc,
                   operation, category,
                   normal_list, other_list);
          }
     }
   else if (cur_desc->as_other)
     {
        if (!category
              || does_idesc_fit_name(ydesc, cur_desc->parent, category)
              || does_idesc_fit_name(ydesc, cur_desc, category)
              || category == cur_desc->individual)
           *other_list = eina_list_append(*other_list, cur_desc);
     }
   else /* Savings */
     {
        if (category && does_idesc_fit_name(ydesc, cur_desc, category))
           *other_list = eina_list_append(*other_list, cur_desc);
     }
}

static Item_Desc *
_find_best_idesc(Year_Desc *ydesc, const char *operation, const char *category)
{
   Item_Desc *idesc = NULL;
   Eina_List *normal_candidates = NULL, *other_candidates = NULL;

   char *loperation = operation?strdup(operation):NULL;
   char *lcategory = category?strdup(category):NULL;
   my_to_lower(loperation, -1);
   my_to_lower(lcategory, -1);
   Eina_Stringshare *operation_shr = eina_stringshare_add(loperation);
   Eina_Stringshare *category_shr = eina_stringshare_add(lcategory);

   _list_candidates(ydesc, ydesc->debits, operation_shr, category_shr, &normal_candidates, &other_candidates);
   _list_candidates(ydesc, ydesc->credits, operation_shr, category_shr, &normal_candidates, &other_candidates);
   _list_candidates(ydesc, ydesc->savings, operation_shr, category_shr, &normal_candidates, &other_candidates);
#if 1
   Eina_List *itr;
   EINA_LIST_FOREACH(normal_candidates, itr, idesc)
     {
        printf("Normal: %s category %s individual %s\n", idesc->name,
              idesc->parent?idesc->parent->name:"(none)",
              idesc->individual?idesc->individual:"(none)");
     }
   EINA_LIST_FOREACH(other_candidates, itr, idesc)
     {
        printf("Other: %s category %s individual %s\n", idesc->name,
              idesc->parent?idesc->parent->name:"(none)",
              idesc->individual?idesc->individual:"(none)");
     }
#endif

   idesc = _item_desc_candidate_guess(normal_candidates, other_candidates);
#if 1
   if (idesc)
     {
        printf("Best: %s category %s individual %s\n", idesc->name,
              idesc->parent?idesc->parent->name:"(none)",
              idesc->individual?idesc->individual:"(none)");
     }
   printf("\n");
#endif

   eina_stringshare_del(category_shr);
   eina_stringshare_del(operation_shr);
   free(lcategory);
   free(loperation);
   return idesc;
}

static Eina_Bool
_chunk_handle(char *chunk, Year_Desc *ydesc, Month_History *hist, float *val)
{
   /*
    * reverse look up float number
    * remains the operation name
    * reverse extract destination (should be at the end) @... if exists
    * stringshare destination and operation and look into items names
    */
   printf("Chunk: %s\n", chunk);
   char *categ = NULL;
   char *categ_setting = NULL;
   Eina_Bool is_minus = EINA_FALSE;
   trailing_remove(chunk);
   float sum = 0.0;
   char *ptr = strrchr(chunk, ' '); /* Look for sum */
   if (!ptr) goto ok;
   sum = atof(ptr + 1);
   *ptr = '\0';

   categ = strchr(chunk, '@');
   if (categ)
     {
        *categ = '\0';
        categ++;
        ptr = categ;
        while (*ptr &&
              ((*ptr >= 'a' && *ptr <= 'z') ||
               (*ptr >= 'A' && *ptr <= 'Z') ||
               (*ptr >= '0' && *ptr <= '9')))
          {
             ptr++;
          }

        if (*ptr == '.') categ_setting = ptr + 1;
        else if (*ptr == '-') is_minus = EINA_TRUE;
        *ptr = '\0';
     }

   trailing_remove(chunk);

   if (*chunk && categ_setting)
     {
        fprintf(stderr, "You cannot set a category while defining an operation\n");
        return EINA_FALSE;
     }
   if (!*chunk) chunk = NULL;

   Item_Desc *idesc = _find_best_idesc(ydesc, chunk, categ);
   if (!idesc)
     {
        fprintf(stderr, "Candidate not found for %s\n", chunk?chunk:categ);
        return EINA_FALSE;
     }

   if (idesc->as_trash) goto ok;
   Month_Item *mitem = month_item_find(hist, idesc);

   if (categ_setting)
     {
        if (!strcmp(categ_setting, "max"))
          {
             if (mitem->max)
               {
                  // FIXME ERROR max already set
                  goto ok;
               }
             mitem->max = sum;
          }
        else if (!strcmp(categ_setting, "expected"))
          {
             if (mitem->expected)
               {
                  // FIXME ERROR expected already set
                  goto ok;
               }
             mitem->expected = sum;
          }
        else if (!strcmp(categ_setting, "init"))
          {
             if (mitem->init)
               {
                  // FIXME ERROR expected already set
                  goto ok;
               }
             mitem->init = sum;
          }
        goto ok;
     }

   Month_Operation *op = calloc(1, sizeof(*op));
   op->v = sum;
   if (!_is_desc_item_named(idesc, chunk))
      op->name = eina_stringshare_add(chunk);
   op->is_minus = is_minus;
   mitem->ops = eina_list_append(mitem->ops, op);

ok:
   *val = sum;
   return EINA_TRUE;
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
   Eina_Bool equal_required = EINA_FALSE;
   Eina_Bool parenthesis_required = EINA_FALSE;
   float line_sum = 0.0;
   Lexer l;
   l.buffer = buffer;
   lexer_reset(&l);

   Month_History *hist = calloc(1, sizeof(*hist));
   hist->month = month;

   do
     {
        if (is_next_token(&l, "("))
          {
             parenthesis_required = EINA_TRUE;
             is_next_token(&l, "\n");
          }
        else if (is_next_token(&l, ")"))
          {
             if (!parenthesis_required)
               {
                  ERROR_PRINT(&l, "Unexpected parenthesis");
                  return EINA_FALSE;
               }

             float given_sum = next_number(&l);
             if ((abs)(given_sum - line_sum) > 0.01)
               {
                  ERROR_PRINT(&l, "Incorrect sum");
                  fprintf(stderr, "Given %.2f Expected %.2f\n", given_sum, line_sum);
                  return EINA_FALSE;
               }
             parenthesis_required = EINA_FALSE;
             line_sum = 0.0;
             is_next_token(&l, "\n");
          }
        else
          {
             char *chunk = chunk_get(&l, EINA_FALSE, '\n', '&', '=', '\0');
             if (chunk)
               {
                  float val = 0.0;
                  if (!_chunk_handle(chunk, ydesc, hist, &val))
                    {
                       ERROR_PRINT(&l, "Chunk error");
                       fprintf(stderr, "%s\n", chunk);
                       return EINA_FALSE;
                    }
                  line_sum += val;
               }
             else goto end;
             if (is_next_token(&l, "&"))
               {
                  equal_required = EINA_TRUE;
               }
             else if (is_next_token(&l, "="))
               {
                  float given_sum = next_number(&l);
                  if ((abs)(given_sum - line_sum) > 0.01)
                    {
                       ERROR_PRINT(&l, "Incorrect sum");
                       fprintf(stderr, "Given %.2f Expected %.2f\n", given_sum, line_sum);
                       return EINA_FALSE;
                    }
                  equal_required = EINA_FALSE;
                  line_sum = 0.0;
               }
             else
               {
                  if (equal_required)
                    {
                       ERROR_PRINT(&l, "'=' required");
                       return EINA_FALSE;
                    }
                  is_next_token(&l, "\n");
                  if (!parenthesis_required) line_sum = 0.0;
               }
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

