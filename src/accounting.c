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

static float
_item_desc_print(Month_History *hist, Item_Desc *idesc, int nb_spaces)
{
   Eina_List *itr;
   Month_Item *mitem = month_item_find(hist, idesc);
   int nb_ops = eina_list_count(mitem->ops);
   Month_Operation *op;
   float item_sum = 0.0;
   printf("%*sItem %s ", nb_spaces, " ", mitem->desc->name);
   if (mitem->max) printf("%*smax %.2f ", nb_spaces, " ", mitem->max);
   if (mitem->expected) printf("%*sexpected %.2f ", nb_spaces, " ", mitem->expected);
   if (nb_ops != 1) printf("\n");
   EINA_LIST_FOREACH(mitem->ops, itr, op)
     {
        item_sum += (op->v * (op->is_minus ? -1 : 1));
        printf("%*s  %.2f", nb_spaces, " ", op->v);
        if (op->name) printf(" (%s)", op->name);
        printf("\n");
     }
   EINA_LIST_FOREACH(idesc->subitems, itr, idesc)
     {
        item_sum += _item_desc_print(hist, idesc, nb_spaces + 2);
     }
   printf("%*s  Total: %.2f\n", nb_spaces, " ", item_sum);
   return item_sum;
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

   Eina_List *itr_m, *itr_i;
   EINA_LIST_FOREACH(ydesc->months, itr_m, hist)
     {
        Item_Desc *idesc;
        float sum = 0;
        printf("Debits: \n");
        EINA_LIST_FOREACH(ydesc->debits, itr_i, idesc)
          {
             sum += _item_desc_print(hist, idesc, 2);
          }
        printf("Total: %.2f\n", sum);
        sum = 0;
        printf("Savings: \n");
        EINA_LIST_FOREACH(ydesc->savings, itr_i, idesc)
          {
             sum += _item_desc_print(hist, idesc, 2);
          }
        printf("Total: %.2f\n", sum);
        sum = 0;
        printf("Credits: \n");
        EINA_LIST_FOREACH(ydesc->credits, itr_i, idesc)
          {
             sum += _item_desc_print(hist, idesc, 2);
          }
        printf("Total: %.2f\n", sum);
     }

   evas_object_show(win);

   elm_run();

   eina_shutdown();
   return 0;
}
ELM_MAIN()
