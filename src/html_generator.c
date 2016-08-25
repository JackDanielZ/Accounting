#include <Eina.h>

#include "common.h"

static const char *header =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<title>Report</title>\n"
"</head>\n"
"<body>\n"
"<table border=\"1\" style=\"width:100%\">"
;

static const char *end =
"</table>\n"
"</body>\n"
"</html>\n"
;

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
        if (hist) sum = idesc_sum_calc(hist, idesc);
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;
        fprintf(fp, "      <td>%d</td>\n", (int)sum);
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

   fprintf(fp, header);

   fprintf(fp, "   <tr>\n      <th></th>\n");
   for (m = 0; m < 12; m++) fprintf(fp, "      <th>%s</th>", _months[m]);
   fprintf(fp, "   </tr>\n");

   EINA_LIST_FOREACH(ydesc->debits, itr, idesc)
     {
        _item_generate(fp, ydesc, idesc, 1);
     }

   fprintf(fp, end);
   fclose(fp);

   return 0;
}

