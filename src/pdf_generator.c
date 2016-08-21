#include <Eina.h>

#include "common.h"

static const char *header =
"\\documentclass{article}\n"
"\\usepackage{adjustbox}\n"
"\\usepackage[table]{xcolor}\n"
"\\usepackage{geometry}\n"
"\\geometry{a4paper, landscape, margin=0in}\n"
"\\begin{document}\n"
"\\pagenumbering{gobble}\n"
"\\rowcolors{2}{white}{yellow}\n"
"\\begin{table}[ht]\n"
"\\centering\n"
"\\noindent\\adjustbox{width=1\\textwidth}{%\n"
"\\begin{tabular}{|p{5cm}|c|c|c|c|c|c|c|c|c|c|c|c|}\n"
"\\hline\n"
;

static const char *end =
"\\end{tabular}}\n"
"\\end{table}\n"
"\\end{document}\n"
;

extern const char *_months[];

int
pdf_generate(Year_Desc *ydesc, const char *output)
{
   char cmd[128];
   const char *tmp_file = "/tmp/accounting.tex";
   FILE *fp = fopen(tmp_file, "w");
   Eina_List *itr;
   Item_Desc *idesc;
   int m;

   fprintf(fp, header);

   for (m = 0; m < 12; m++) fprintf(fp, "& %s ", _months[m]);
   fprintf(fp, "\\\\\n\\hline\n");

   EINA_LIST_FOREACH(ydesc->debits, itr, idesc)
     {
        fprintf(fp, idesc->name);
        for (m = 0; m < 12; m++)
          {
             Month_History *hist = month_hist_get(ydesc, m);
             float sum = 0.0;
             if (hist) sum = idesc_sum_calc(hist, idesc);
             if (sum - (int)sum > 0.5) sum = (int)sum + 1;
             else sum = (int)sum;
             fprintf(fp, " & %d", (int)sum);
          }
        fprintf(fp, "\\\\\n\\hline\n");
     }

   fprintf(fp, end);
   fclose(fp);

   sprintf(cmd, "texi2pdf -q -c %s -o %s", tmp_file, output);
   return !!system(cmd);
}

