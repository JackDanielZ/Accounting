#include <Eina.h>

#include "common.h"

static const char *header =
"\\documentclass{article}\n"
"\\usepackage{adjustbox}\n"
"\\usepackage[table]{xcolor}\n"
"\\usepackage{geometry}\n"
"\\usepackage{multirow}\n"
"\\geometry{a4paper, landscape, margin=0in}\n"
"\\begin{document}\n"
"\\pagenumbering{gobble}\n"
"\\rowcolors{2}{white}{yellow}\n"
"\\begin{table}[ht]\n"
"\\centering\n"
"\\begin{tabular}{|b{5cm}|c|c|c|c|c|c|c|c|c|c|c|c|}\n"
"\\hline\n"
;

static const char *end =
"\\end{tabular}\n"
"\\end{table}\n"
"\\end{document}\n"
;

extern const char *_months[];

static void
_item_generate(FILE *fp, Year_Desc *ydesc, Item_Desc *idesc, int level)
{
   Eina_List *itr;
   int m;
   fprintf(fp, "\\hspace{%dem}%s", level, idesc->name);
   for (m = 0; m < 12; m++)
     {
        Month_History *hist = month_hist_get(ydesc, m);
        float sum = 0.0;
        if (hist) sum = idesc_sum_calc(ydesc, hist, idesc, NULL, CALC_BASIC, NULL, NULL);
        if (sum - (int)sum > 0.5) sum = (int)sum + 1;
        else sum = (int)sum;
        fprintf(fp, " & %d", (int)sum);
     }
   fprintf(fp, "\\\\\n\\hline\n");
   EINA_LIST_FOREACH(idesc->subitems, itr, idesc)
     {
        _item_generate(fp, ydesc, idesc, level + 1);
     }
}

int
pdf_generate(Year_Desc *ydesc, const char *output)
{
   char cmd[128];
   const char *tmp_file = "/tmp/accounting.tex";
   FILE *fp = fopen(tmp_file, "w");
   int m;

   fprintf(fp, header);

   for (m = 0; m < 12; m++) fprintf(fp, "& %s ", _months[m]);
   fprintf(fp, "\\\\\n\\hline\n");

   //fprintf(fp, "\\multirow{1}{*}{Debits} \\\\\n");
   fprintf(fp, "Debits\n");
   for (m = 0; m < 12; m++) fprintf(fp, " & ");
   fprintf(fp, "\\\\\n");
   _item_generate(fp, ydesc, ydesc->debits, 1);

   fprintf(fp, end);
   fclose(fp);

   sprintf(cmd, "texi2pdf -q -c %s -o %s", tmp_file, output);
   return !!system(cmd);
}

