#include <Eina.h>

#include "common.h"

extern const char *_months[];

static void
_item_generate(FILE *fp, Year_Desc *ydesc, Item_Desc *idesc, int level)
{
   Eina_List *itr;
   int m;
   fprintf(fp, "   <tr>\n");
   fprintf(fp, "      <th>%s</th>\n", idesc->name);
   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        float sum = 0.0;
        Eina_Strbuf *tooltip = eina_strbuf_new();
        if (hist) sum = idesc_sum_calc(hist, idesc, tooltip);
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;
        fprintf(fp, "      <td title=\"%s\">%d</td>\n",
              eina_strbuf_string_get(tooltip),
              (int)sum);
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
   Eina_List *itr;
   Item_Desc *idesc;
   int m;

   char *buffer = file_get_as_string(PACKAGE_DATA_DIR"header_html");
   fprintf(fp, buffer);
   free(buffer);

   fprintf(fp, "   <tr>\n      <th></th>\n");
   for (m = 0; m < 12; m++) fprintf(fp, "      <th>%s</th>", _months[m]);
   fprintf(fp, "   </tr>\n");

   EINA_LIST_FOREACH(ydesc->debits, itr, idesc)
     {
        _item_generate(fp, ydesc, idesc, 1);
     }

   buffer = file_get_as_string(PACKAGE_DATA_DIR"end_html");
   fprintf(fp, buffer);
   fclose(fp);

   return 0;
}

