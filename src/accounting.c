#include <stdio.h>
#include <libgen.h>

#include <Elementary.h>

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

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   char history_file[256];
   char *buffer, *dir;
   Month_History *hist = NULL;
   int m;
   eina_init();

   if (argc <= 1)
     {
        ERR("Usage: %s desc_file", argv[0]);
        exit(1);
     }
   buffer = _file_get_as_string(argv[1]);
   if (!buffer) exit(1);
   Year_Desc *ydesc = desc_parse(buffer);
   free(buffer);

   dir = dirname(argv[1]);
   for (m = 1; m <= 12; m++)
     {
        sprintf(history_file, "%s/History_%d_%.2d.txt", dir, ydesc->year, m);
        buffer = _file_get_as_string(history_file);
        if (buffer)
          {
             history_parse(buffer, m, ydesc);
             free(buffer);
          }
     }

   Eina_List *itr_m, *itr_i, *itr_o;
   EINA_LIST_FOREACH(ydesc->months, itr_m, hist)
     {
        Month_Item *mitem;
        EINA_LIST_FOREACH(hist->items, itr_i, mitem)
          {
             int nb_ops = eina_list_count(mitem->ops);
             Month_Operation *op;
             printf("Item %s ", mitem->desc->name);
             if (mitem->max) printf("max %.2f ", mitem->max);
             if (mitem->expected) printf("expected %.2f ", mitem->expected);
             if (nb_ops != 1) printf("\n");
             EINA_LIST_FOREACH(mitem->ops, itr_o, op)
               {
                  printf("  %.2f", op->v);
                  if (op->name) printf(" (%s)", op->name);
                  printf("\n");
               }
          }
     }

   eina_shutdown();
   return 0;
}
ELM_MAIN()
