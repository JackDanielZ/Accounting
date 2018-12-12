#include <libgen.h>
#include <getopt.h>

#include "common.h"

enum
{
   GEN_HTML
};

int
main(int argc, char **argv)
{
   char history_file[256];
   char *buffer;
   int m = 0, opt, gen_what = GEN_HTML, ret = 0;
   int help = 0;

   struct option opts[] = {
      { "help",    no_argument,       NULL,       'h'        },
      { "html",    no_argument,       &gen_what,  GEN_HTML   },
      { NULL, 0, NULL, 0 }
   };

   if (argc <= 1)
     {
        ERR("Usage: %s [--html] desc_file", argv[0]);
        return(1);
     }

   for (opt = 0; (opt = getopt_long(argc, argv, "ho:", opts, NULL)) != -1; )
     {
        switch (opt)
          {
           case 0: break;
           case 'h':
                   help = 1;
                   break;
           default:
                   help = 1;
                   break;
          }
     }

   if (help)
     {
        printf("Usage: %s [-h/--help] [--html] [--output/-o outfile] desc_file ... \n", argv[0]);
        printf("       --help/-h Print that help\n");
        printf("       --html Generate a HTML output\n");
        goto end;
     }
   char *desc_file = argv[optind++];
   buffer = file_get_as_string(desc_file);
   if (!buffer) return(1);
   Year_Desc *ydesc = desc_parse(buffer);
   free(buffer);

   ydesc->files_dir = dirname(desc_file);
   while (1)
     {
        sprintf(history_file, "%s/History_%d_%.2d.txt", ydesc->files_dir, ydesc->year, m + 1);
        buffer = file_get_as_string(history_file);
        if (buffer)
          {
             if (!history_parse(buffer, m, ydesc))
               {
                  fprintf(stderr, "Parsing of %s failed\n", history_file);
                  ret = 1;
                  goto end;
               }
             free(buffer);
          }
        else break;
        m++;
     }

   if (gen_what == GEN_HTML) ret = html_generate(ydesc, "toto.html");
end:
   return ret;
}
