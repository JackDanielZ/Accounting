#include <Eina.h>

#include "common.h"

extern const char *_months[];

static int _row_number = 0;

static void
_item_generate(FILE *fp, Year_Desc *ydesc, Item_Desc *idesc, int level)
{
   Eina_List *itr;
   int m, nb_children = eina_list_count(idesc->subitems);
   if (idesc->as_trash) return;
   fprintf(fp, "   <tr class=\"d%d\" expanded=0 level=%d%s>\n",
         _row_number++ % 2,
         level, level?" style=\"display:none;\"":"");
   fprintf(fp, "      <th>");
   if (nb_children)
      fprintf(fp, "      <button %s onclick=\"toggleRow(this);\">+</button>",
            !nb_children ? " style=\"display:none;\"":"");
   fprintf(fp, "      </th><th>%s</th>\n", idesc->name);

   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        Month_Item *mitem = month_item_find(hist, idesc);
        float sum = 0.0, expected = 0.0;
        Eina_Strbuf *tooltip = eina_strbuf_new();
        if (hist) sum = idesc_sum_calc(hist, idesc, tooltip, CALC_ALL, &expected);
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;
        fprintf(fp, "      <td title=\"%s\">%s%d%s</td>\n",
              eina_strbuf_string_get(tooltip),
              mitem && mitem->expected?"<i>":"",
              (int)sum,
              mitem && mitem->expected?"</i>":"");
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
   fprintf(fp, buffer);
   free(buffer);

   _row_number = 0;
   fprintf(fp, "   <tr class=\"d%d\">\n      <th></th><th></th>\n", _row_number++ % 2);
   for (m = 0; m < 12; m++) fprintf(fp, "      <th>%s</th>", _months[m]);
   fprintf(fp, "   </tr>\n");

   _item_generate(fp, ydesc, ydesc->debits, 0);
   _item_generate(fp, ydesc, ydesc->savings, 0);
   _item_generate(fp, ydesc, ydesc->credits, 0);

   fprintf(fp, "   <tr class=\"d%d\">\n      <th></th><th>Remaining</th>\n", _row_number++ % 2);
   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        float sum = 0.0;
        if (hist)
          {
             sum += idesc_sum_calc(hist, ydesc->credits, NULL, CALC_ALL, NULL);
             sum -= idesc_sum_calc(hist, ydesc->debits, NULL, CALC_ALL, NULL);
             sum -= idesc_sum_calc(hist, ydesc->savings, NULL, CALC_POSITIVE, NULL);
          }
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;
        fprintf(fp, "      <td title=\"%s\">%d</td>\n", "", (int)sum);
     }
   fprintf(fp, "   </tr>\n");

   buffer = file_get_as_string(PACKAGE_DATA_DIR"end_html");
   fprintf(fp, buffer);
   fclose(fp);

   return 0;
}

