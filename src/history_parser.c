#include "common.h"

static int
_hist_parse(const char *buffer, Month_History *hist, Year_Desc *ydesc);

static int
_is_desc_item_named(Item_Desc *idesc, const char *name)
{
   List *itr;
   const char *nick;
   if (!idesc) return 0;
   char *lname = strdup(name);
   my_to_lower(lname, -1);
   int ret = 1;
   LIST_FOREACH(idesc->nicknames, itr, nick)
     {
        if (!strcmp(nick, lname)) goto end;
     }
   ret = 0;
end:
   free(lname);
   return ret;
}

static Item_Desc *
_item_desc_candidate_guess(List *normal_candidates, List *other_candidates)
{
   List *itr;
   Item_Desc *idesc, *found = NULL;

   if (normal_candidates)
     {
        if (list_count(normal_candidates) == 1) found = list_data_get(normal_candidates);
        LIST_FOREACH(normal_candidates, itr, idesc)
          {
             if (found) continue;
             if (!idesc->parent || !idesc->parent->parent) found = idesc;
          }
     }

   if (!found && list_count(other_candidates) == 1) found = list_data_get(other_candidates);
   LIST_FOREACH(other_candidates, itr, idesc)
     {
        if (found) continue;
        if (!idesc->parent || !idesc->parent->parent) found = idesc;
     }
   return found;
}

static void
_list_candidates(Year_Desc *ydesc, Item_Desc *cur_desc,
      const char *operation, const char *category,
      List **normal_list, List **other_list)
{
   List *itr;
   Item_Desc *sub_desc;
   if (!cur_desc) return;
   if (!cur_desc->as_other && !cur_desc->as_saving)
     {
        if (operation)
          {
             if (does_idesc_fit_name(ydesc, cur_desc, operation))
               {
                  if (!category
                        || does_idesc_fit_name(ydesc, cur_desc->parent, category)
                        || (cur_desc->individual && !strcmp(category, cur_desc->individual)))
                     *normal_list = list_append(*normal_list, cur_desc);
               }
             else
               {
                  /* e.g Conversion @Bank 300 */
                  if (category && !cur_desc->subitems && does_idesc_fit_name(ydesc, cur_desc, category))
                     *normal_list = list_append(*normal_list, cur_desc);
               }
          }
        else
          {
             /* e.g SV.init 40 */
             if (category && does_idesc_fit_name(ydesc, cur_desc, category))
                *normal_list = list_append(*normal_list, cur_desc);
          }


        LIST_FOREACH(cur_desc->subitems, itr, sub_desc)
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
              || (cur_desc->individual && !strcmp(category, cur_desc->individual)))
           *other_list = list_append(*other_list, cur_desc);
     }
   else /* Savings */
     {
        if (category && does_idesc_fit_name(ydesc, cur_desc, category))
           *other_list = list_append(*other_list, cur_desc);
     }
}

static Item_Desc *
_find_best_idesc(Year_Desc *ydesc, const char *operation, const char *category)
{
   Item_Desc *idesc = NULL;
   List *normal_candidates = NULL, *other_candidates = NULL;

   char *loperation = operation?strdup(operation):NULL;
   char *lcategory = category?strdup(category):NULL;
   my_to_lower(loperation, -1);
   my_to_lower(lcategory, -1);

   _list_candidates(ydesc, ydesc->debits, loperation, lcategory, &normal_candidates, &other_candidates);
   _list_candidates(ydesc, ydesc->credits, loperation, lcategory, &normal_candidates, &other_candidates);
   _list_candidates(ydesc, ydesc->savings, loperation, lcategory, &normal_candidates, &other_candidates);
#if 1
   List *itr;
   LIST_FOREACH(normal_candidates, itr, idesc)
     {
        printf("Normal: %s category %s individual %s\n", idesc->name,
              idesc->parent?idesc->parent->name:"(none)",
              idesc->individual?idesc->individual:"(none)");
     }
   LIST_FOREACH(other_candidates, itr, idesc)
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

   free(lcategory);
   free(loperation);
   return idesc;
}

static int
_chunk_handle(char *chunk, Year_Desc *ydesc, Month_History *hist, double *val)
{
   /*
    * reverse look up double number
    * remains the operation name
    * reverse extract destination (should be at the end) @... if exists
    * stringshare destination and operation and look into items names
    */
   printf("Chunk: %s\n", chunk);
   char *categ = NULL, *comment;
   char *categ_setting = NULL;
   int is_minus = 0;
   trailing_remove(chunk);
   double sum = 0.0;
   if (!strcmp(chunk, "@simulation"))
     {
        hist->simulation = 1;
        goto ok;
     }
   if (!strncmp(chunk, "@import", 7))
     {
        char history_file[256];
        chunk += 8;
        sprintf(history_file, "%s/%s", ydesc->files_dir, chunk);
        char *buffer = file_get_as_string(history_file);
        if (!_hist_parse(buffer, hist, ydesc)) return 0;
        goto ok;
     }
   char *ptr = strrchr(chunk, ' '); /* Look for sum */
   if (!ptr) goto ok;
   sum = atof(ptr + 1);
   *ptr = '\0';

   trailing_remove(chunk);
   comment = strstr(chunk, "--");
   if (comment)
     {
        *comment = '\0';
        comment += 2;
        while (*comment && *comment == ' ') comment++;
     }

   trailing_remove(chunk);
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
        else if (*ptr == '-') is_minus = 1;
        *ptr = '\0';
     }

   trailing_remove(chunk);

   if (*chunk && categ_setting)
     {
        fprintf(stderr, "You cannot set a category while defining an operation\n");
        return 0;
     }
   if (!*chunk) chunk = NULL;

   Item_Desc *idesc = _find_best_idesc(ydesc, chunk, categ);
   if (!idesc)
     {
        fprintf(stderr, "Candidate not found for %s\n", chunk?chunk:categ);
        return 0;
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
      op->name = strdup(chunk);
   op->comment = comment?strdup(comment):NULL;
   op->is_minus = is_minus;
   mitem->ops = list_append(mitem->ops, op);

ok:
   *val = sum;
   return 1;
}

static int
_hist_parse(const char *buffer, Month_History *hist, Year_Desc *ydesc)
{
   /*
    * if token == (
    *   extract chunk until & or )
    * else extract chunk until \0 or \n
    * for each chunk
    */
   int equal_required = 0;
   int parenthesis_required = 0;
   double line_sum = 0.0;
   Lexer l;
   l.buffer = buffer;
   lexer_reset(&l);

   do
     {
        if (is_next_token(&l, "("))
          {
             parenthesis_required = 1;
             is_next_token(&l, "\n");
          }
        else if (is_next_token(&l, ")"))
          {
             if (!parenthesis_required)
               {
                  ERROR_PRINT(&l, "Unexpected parenthesis");
                  return 0;
               }

             double given_sum = next_number(&l);
             if ((abs)(given_sum - line_sum) > 0.01)
               {
                  ERROR_PRINT(&l, "Incorrect sum");
                  fprintf(stderr, "Given %.2f Expected %.2f\n", given_sum, line_sum);
                  return 0;
               }
             parenthesis_required = 0;
             line_sum = 0.0;
             is_next_token(&l, "\n");
          }
        else
          {
             char *chunk = chunk_get(&l, 0, '\n', '&', '=', '\0');
             if (chunk)
               {
                  double val = 0.0;
                  if (!_chunk_handle(chunk, ydesc, hist, &val))
                    {
                       ERROR_PRINT(&l, "Chunk error");
                       fprintf(stderr, "%s\n", chunk);
                       return 0;
                    }
                  line_sum += val;
               }
             else goto end;
             if (is_next_token(&l, "&"))
               {
                  equal_required = 1;
               }
             else if (is_next_token(&l, "="))
               {
                  double given_sum = next_number(&l);
                  if ((abs)(given_sum - line_sum) > 0.01)
                    {
                       ERROR_PRINT(&l, "Incorrect sum");
                       fprintf(stderr, "Given %.2f Expected %.2f\n", given_sum, line_sum);
                       return 0;
                    }
                  equal_required = 0;
                  line_sum = 0.0;
               }
             else
               {
                  if (equal_required)
                    {
                       ERROR_PRINT(&l, "'=' required");
                       return 0;
                    }
                  is_next_token(&l, "\n");
                  if (!parenthesis_required) line_sum = 0.0;
               }
          }
     }
   while (1);

end:
   ydesc->months = list_append(ydesc->months, hist);
   return 1;
}

int
history_parse(const char *buffer, int month, Year_Desc *ydesc)
{
   Month_History *hist = calloc(1, sizeof(*hist));
   hist->month = month;
   return _hist_parse(buffer, hist, ydesc);
}
