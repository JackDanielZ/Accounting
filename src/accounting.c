#include <stdio.h>
#include <libgen.h>
#include <getopt.h>

#include <Elementary.h>

#include "ui.h"
#include "common.h"

static void
_my_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit(); /* exit the program's main loop that runs in elm_run() */
}

enum
{
   GEN_UI,
   GEN_PDF,
   GEN_HTML
};

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   char history_file[256];
   char *buffer, *dir;
   int m, opt, gen_what = GEN_UI, ret = 0;
   Eina_Bool help = EINA_FALSE;
   eina_init();

   struct option opts[] = {
      { "help",    no_argument,       NULL,       'h'        },
      { "pdf",     no_argument,       &gen_what,  GEN_PDF    },
      { "html",    no_argument,       &gen_what,  GEN_HTML   },
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
   buffer = file_get_as_string(desc_file);
   if (!buffer) exit(1);
   Year_Desc *ydesc = desc_parse(buffer);
   free(buffer);

   dir = dirname(desc_file);
   for (m = 0; m < 12; m++)
     {
        sprintf(history_file, "%s/History_%d_%.2d.txt", dir, ydesc->year, m + 1);
        buffer = file_get_as_string(history_file);
        if (buffer)
          {
             history_parse(buffer, m, ydesc);
             free(buffer);
          }
     }

   if (gen_what == GEN_PDF)
     {
        ret = pdf_generate(ydesc, "toto.tex");
        goto end;
     }
   if (gen_what == GEN_HTML)
     {
        ret = html_generate(ydesc, "toto.html");
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
   return ret;
}
ELM_MAIN()
