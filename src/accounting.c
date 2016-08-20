#include <stdio.h>
#include <libgen.h>

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

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   char history_file[256];
   char *buffer, *dir;
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

   Eo *win = elm_win_util_standard_add("Accounting", "Accounting");
   evas_object_smart_callback_add(win, "delete,request", _my_win_del, NULL);
   evas_object_resize(win, 1200, 768);
   elm_win_maximized_set(win, EINA_TRUE);

   ui_year_create(ydesc, win);

   evas_object_show(win);

   elm_run();

   eina_shutdown();
   return 0;
}
ELM_MAIN()
