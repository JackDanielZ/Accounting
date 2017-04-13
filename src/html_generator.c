#include <Eina.h>

#include "common.h"

extern const char *_months[];

static int _row_number = 0;

static void
_item_generate(FILE *fp, Year_Desc *ydesc, Item_Desc *idesc, int level)
{
   Eina_List *itr;
   if (!idesc) return;
   int m, nb_children = eina_list_count(idesc->subitems);
   if (idesc->as_trash) return;
   fprintf(fp, "   <tr class=\"d%d\" expanded=0 level=%d%s>\n",
         _row_number++ % 2,
         level, level?" style=\"display:none;\"":"");
   fprintf(fp, "      <th>");
   if (nb_children)
      fprintf(fp, "      <button %s onclick=\"toggleRow(this);\">+</button>",
            !nb_children ? " style=\"display:none;\"":"");
   fprintf(fp, "      </th><th style=\"text-indent:%dpx\">%s", level*20, idesc->name);
   if (idesc->individual)
     {
        Item_Desc *ind_desc = individual_find(ydesc, idesc->individual);
        fprintf(fp, " (%s)", ind_desc->name);
     }
   fprintf(fp, "</th>\n");

   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        Month_Item *mitem = month_item_find(hist, idesc);
        float sum = 0.0, expected = 0.0;
        Eina_Strbuf *tooltip = eina_strbuf_new();
        if (hist) sum = idesc_sum_calc(ydesc, hist, idesc, tooltip, CALC_BASIC | CALC_INDIVIDUALS, NULL, &expected);
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;

        fprintf(fp, "      <td title=\"%s\"", eina_strbuf_string_get(tooltip));
        if (mitem)
          {
             fprintf(fp, "%s>%s%d%s",
                   mitem->max && sum > mitem->max ?"style=\"color:red\"><b" : "",
                   mitem->expected?"<i>":"", (int)sum, mitem->expected?"</i>":"");
             if (mitem->max)
               {
                  fprintf(fp, " / %d", (int)mitem->max);
               }
             if (mitem->max && sum > mitem->max) fprintf(fp, "</b>");
          }
        else
          {
             fprintf(fp, ">");
          }
        fprintf(fp, "</td>\n");
     }
   fprintf(fp, "   </tr>\n");
   EINA_LIST_FOREACH(idesc->subitems, itr, idesc)
     {
        _item_generate(fp, ydesc, idesc, level + 1);
     }
}

int
html_generate(Year_Desc *ydesc, const char *output)
{
   FILE *fp = fopen(output, "w");
   int m;
   char *buffer = file_get_as_string(PACKAGE_DATA_DIR"header_html");
   Eina_List *itr;
   Item_Desc *idesc;

   fprintf(fp, buffer);
   free(buffer);

   _row_number = 0;
   fprintf(fp, "   <tr class=\"d%d\">\n      <th></th><th></th>\n", _row_number++ % 2);
   for (m = 0; m < 12; m++) fprintf(fp, "      <th>%s</th>", _months[m]);
   fprintf(fp, "   </tr>\n");

   _item_generate(fp, ydesc, ydesc->debits, 0);
   _item_generate(fp, ydesc, ydesc->savings, 0);
   _item_generate(fp, ydesc, ydesc->credits, 0);

   fprintf(fp, "   <tr class=\"d%d\">\n      <th>", _row_number++ % 2);
   if (ydesc->individuals)
      fprintf(fp, "      <button onclick=\"toggleRow(this);\">+</button>");
   fprintf(fp, "</th><th>Remaining</th>\n");
   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        float credits, sum = 0.0, expected_debits = 0.0;
        float all_debits, all_savings;
        if (hist)
          {
             /* Credits + debits - savings (only the savings, i.e positive, not the expenses */
             credits = idesc_sum_calc(ydesc, hist, ydesc->credits, NULL, CALC_BASIC | CALC_INDIVIDUALS, NULL, NULL);
             all_debits = idesc_sum_calc(ydesc, hist, ydesc->debits, NULL, CALC_BASIC | CALC_INDIVIDUALS, NULL, &expected_debits);
             all_savings = idesc_sum_calc(ydesc, hist, ydesc->savings, NULL, CALC_POSITIVE | CALC_INDIVIDUALS, NULL, NULL);
             sum = credits - all_debits - all_savings;
          }
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        if (expected_debits - (int)expected_debits > 0.5) expected_debits = (int)expected_debits + 1;
        else sum = (int)sum;
        if (!expected_debits)
           fprintf(fp, "      <td>%d</td>\n", (int)sum);
        else
           fprintf(fp, "      <td>%d (-%d)</td>\n", (int)sum, (int)expected_debits);
     }
   fprintf(fp, "   </tr>\n");
   EINA_LIST_FOREACH(ydesc->individuals?ydesc->individuals->subitems:NULL, itr, idesc)
     {
        fprintf(fp, "   <tr class=\"d%d\" expanded=0 level=0>\n      <th></th><th>%s</th>\n",
              _row_number++ % 2, idesc->name);
        for (m = 0; m < 12; m++)
          {
             Month_History *hist = month_hist_get(ydesc, m);
             float sum = 0.0;
             if (hist)
               {
                  float own_credits = idesc_sum_calc(ydesc, hist, ydesc->credits,
                        NULL, CALC_BASIC, idesc->name, NULL);
                  float common_debits = idesc_sum_calc(ydesc, hist, ydesc->debits,
                        NULL, CALC_BASIC, NULL, NULL);
                  float common_savings = idesc_sum_calc(ydesc, hist, ydesc->savings,
                        NULL, CALC_POSITIVE, NULL, NULL);
                  float own_debits = idesc_sum_calc(ydesc, hist, ydesc->debits,
                        NULL, CALC_BASIC, idesc->name, NULL);
                  float own_savings = idesc_sum_calc(ydesc, hist, ydesc->savings,
                        NULL, CALC_POSITIVE, idesc->name, NULL);
                  sum = own_credits - (common_debits / 2) - (common_savings / 2) -
                     own_debits - own_savings;
               }
             fprintf(fp, "      <td>%d</td>\n", (int)sum);
          }
        fprintf(fp, "   </tr>\n");
     }

   buffer = file_get_as_string(PACKAGE_DATA_DIR"end_html");
   fprintf(fp, buffer);
   fclose(fp);

   return 0;
}

