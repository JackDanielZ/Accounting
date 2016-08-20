#include <stdio.h>
#include <libgen.h>
#include <getopt.h>

#include <Elementary.h>

#include "ui.h"
#include "common.h"

#define ERR(fmt, ...) fprintf(stderr, fmt"\n", ## __VA_ARGS__)

static char*
_file_get_as_string(const char *filename)
{
   char *file_data = NULL;
   int file_size;
   FILE *fp = fopen(filename, "r");
   if (!fp)
     {
        ERR("Can not open file: \"%s\".", filename);
        return NULL;
     }

   fseek(fp, 0, SEEK_END);
   file_size = ftell(fp);
   if (file_size == -1)
     {
        fclose(fp);
        ERR("Can not ftell file: \"%s\".", filename);
        return NULL;
     }
   rewind(fp);
   file_data = (char *) calloc(1, file_size + 1);
   if (!file_data)
     {
        fclose(fp);
        ERR("Calloc failed");
        return NULL;
     }
   int res = fread(file_data, file_size, 1, fp);
   fclose(fp);
   if (!res)
     {
        free(file_data);
        file_data = NULL;
        ERR("fread failed");
     }
   return file_data;
}

static void
_my_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit(); /* exit the program's main loop that runs in elm_run() */
}

enum
{
   GEN_UI,
   GEN_PDF
};

Eina_Bool
_pdf_generate(Year_Desc *ydesc, const char *output)
{
   char cmd[128];
   const char *tmp_file = "/tmp/accounting.tex";
   FILE *fp = fopen(tmp_file, "w");

   fprintf(fp,
         "\\documentclass{article}\n"
         "\\begin{document}\n"
         "\\begin{table}[ht]\n"
         "\\centering\n"
         "\\begin{tabular}{c|ccccccc}\n"
         "\\hline\n"
         "& col1 & col2 & col3 & col4 & col5 & col6 & col7 \\\\\n"
         "\\hline\n"
         "row1& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "row2& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "row3& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "row4& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "row5& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "row6& 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 & 0.9 \\\\\n"
         "\\hline\n"
         "\\end{tabular}\n"
         "\\end{table}\n"
         "\\end{document}\n");
   fclose(fp);

   sprintf(cmd, "texi2pdf -q -c %s -o %s", tmp_file, output);
   system(cmd);
   return EINA_TRUE;
}

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   char history_file[256];
   char *buffer, *dir;
   int m, opt, gen_what = GEN_UI;
   Eina_Bool help = EINA_FALSE;
   eina_init();

   struct option opts[] = {
      { "help",    no_argument,       NULL,       'h'        },
      { "pdf",     no_argument,       &gen_what,  GEN_PDF    },
//      { "output",  required_argument, NULL,       'o'        },
      { NULL, 0, NULL, 0 }
   };

   if (argc <= 1)
     {
        ERR("Usage: %s [--pdf] desc_file", argv[0]);
        exit(1);
     }

   for (opt = 0; (opt = getopt_long(argc, argv, "ho:", opts, NULL)) != -1; )
     {
        switch (opt)
          {
           case 0: break;
#if 0
           case 'o':
                   outf = optarg;
                   break;
#endif
           case 'h':
                   help = EINA_TRUE;
                   break;
           default:
                   help = EINA_TRUE;
                   break;
          }
     }

   if (help)
     {
        printf("Usage: %s [-h/--help] [--pdf] [--output/-o outfile] desc_file ... \n", argv[0]);
        printf("       --help/-h Print that help\n");
        printf("       --pdf Generate a PDF output\n");
        goto end;
     }
   char *desc_file = argv[optind++];
   buffer = _file_get_as_string(desc_file);
   if (!buffer) exit(1);
   Year_Desc *ydesc = desc_parse(buffer);
   free(buffer);

   dir = dirname(desc_file);
   for (m = 0; m < 12; m++)
     {
        sprintf(history_file, "%s/History_%d_%.2d.txt", dir, ydesc->year, m + 1);
        buffer = _file_get_as_string(history_file);
        if (buffer)
          {
             history_parse(buffer, m, ydesc);
             free(buffer);
          }
     }

   if (gen_what == GEN_PDF)
     {
        _pdf_generate(ydesc, "toto.tex");
        goto end;
     }
   Eo *win = elm_win_util_standard_add("Accounting", "Accounting");
   evas_object_smart_callback_add(win, "delete,request", _my_win_del, NULL);
   evas_object_resize(win, 1200, 768);
   elm_win_maximized_set(win, EINA_TRUE);

   ui_year_create(ydesc, win);

   evas_object_show(win);

   elm_run();

end:
   eina_shutdown();
   return 0;
}
ELM_MAIN()
