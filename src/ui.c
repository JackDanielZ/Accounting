#include <Elementary.h>
#include <Eo.h>

#include "ui.h"

EOAPI void *eo_key_data_get(const Eo *obj, const char * key);
EOAPI void eo_key_data_set(Eo *obj, const char * key, const void *data);

Elm_Genlist_Item_Class *gitc = NULL, *itc = NULL;

enum
{
   ITEM_MONTHS,
   ITEM_DEBITS,
   ITEM_CREDITS,
   ITEM_SAVINGS
};

static const char *_months[] =
{
   "January", "February", "March",
   "April", "May", "June", "July",
   "August", "September", "October",
   "November", "December"
};

static void
_expand(void *data EINA_UNUSED, Evas_Object *cont, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   Eina_List *idesc_list = eo_key_data_get(glit, "idesc_list");
   Item_Desc *idesc = elm_object_item_data_get(glit);
   Eina_List *itr, *lst = idesc_list ? idesc_list : idesc->subitems;
   EINA_LIST_FOREACH(lst, itr, idesc)
     {
        if (idesc->as_trash) continue;
        elm_genlist_item_append(cont, itc, idesc, glit,
              idesc->subitems ? ELM_GENLIST_ITEM_TREE : ELM_GENLIST_ITEM_NONE,
              NULL, NULL);
     }
}

static void
_contract(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_subitems_clear(glit);
}

static void
_expand_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_TRUE);
}

static void
_contract_req(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *glit = event_info;
   elm_genlist_item_expanded_set(glit, EINA_FALSE);
}

static char *
_group_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   int item_type = (uintptr_t)data;
   switch(item_type)
     {
      case ITEM_MONTHS: return NULL;
      case ITEM_DEBITS: return strdup("Debits");
      case ITEM_CREDITS: return strdup("Credits");
      case ITEM_SAVINGS: return strdup("Savings");
     }
   return NULL;
}

static Evas_Object *
_group_content_get(void *data, Evas_Object *gl, const char *part)
{
   Evas_Object *box = NULL;
   if (!strcmp(part, "elm.swallow.end"))
     {
        int i;
        int item_type = (uintptr_t)data;
        //Year_Desc *ydesc = eo_key_data_get(gl, "ydesc");
        box = elm_box_add(gl);
        elm_box_homogeneous_set(box, EINA_TRUE);
        elm_box_horizontal_set(box, EINA_TRUE);
        if (item_type == ITEM_MONTHS)
          {
             for (i = 0; i < 12; i++)
               {
                  Eo *obj = elm_label_add(box);
                  elm_object_text_set(obj, _months[i]);
                  evas_object_size_hint_min_set(obj, 120, 140);
                  elm_box_pack_end(box, obj);
                  evas_object_show(obj);
               }
          }
     }
   return box;
}

static char *
_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   Item_Desc *idesc = data;
   return strdup(idesc->name);
}

static Month_History *
_month_hist_get(Year_Desc *ydesc, int month)
{
   Eina_List *itr;
   Month_History *hist = NULL;
   EINA_LIST_FOREACH(ydesc->months, itr, hist)
     {
        if (hist->month == month) return hist;
     }
   return hist;
}

static Evas_Object *
_item_content_get(void *data, Evas_Object *gl, const char *part)
{
   Evas_Object *box = NULL;
   if (!strcmp(part, "elm.swallow.end"))
     {
        Item_Desc *idesc = data;
        Eina_List *itr;
        Year_Desc *ydesc = eo_key_data_get(gl, "ydesc");
        int i;
        box = elm_box_add(gl);
        elm_box_horizontal_set(box, EINA_TRUE);
        for (i = 0; i < 12; i++)
          {
             char buf[16];
             Month_History *hist = _month_hist_get(ydesc, i);
             float sum = 0.0;
             if (hist)
               {
                  Month_Item *mitem = month_item_find(hist, idesc);
                  Month_Operation *op;
                  EINA_LIST_FOREACH(mitem->ops, itr, op)
                    {
                       sum += (op->v * (op->is_minus ? -1 : 1));
                    }
               }
             Eo *obj = elm_label_add(box);
             elm_label_ellipsis_set(obj, EINA_TRUE);
             if (sum - (int)sum > 0.5) sum = (int)sum + 1;
             else sum = (int)sum;
             sprintf(buf, "%d", (int)sum);
             elm_object_text_set(obj, buf);
             evas_object_size_hint_min_set(obj, 120, 140);
             elm_box_pack_end(box, obj);
             evas_object_show(obj);
          }
     }
   return box;
}

Eo *
ui_year_create(Year_Desc *ydesc, Eo *win)
{
   Elm_Object_Item *git = NULL;

   Eo *cont = elm_genlist_add(win);
   eo_key_data_set(cont, "ydesc", ydesc);
   evas_object_size_hint_weight_set(cont, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cont, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(win, cont);
   elm_genlist_homogeneous_set(cont, EINA_TRUE);
   evas_object_show(cont);
   ydesc->ui_data = cont;

   gitc = elm_genlist_item_class_new();
   gitc->item_style = "default";
   gitc->func.text_get = _group_text_get;
   gitc->func.content_get = _group_content_get;

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = _item_text_get;
   itc->func.content_get = _item_content_get;

   evas_object_smart_callback_add(cont, "expand,request", _expand_req, NULL);
   evas_object_smart_callback_add(cont, "contract,request", _contract_req, NULL);
   evas_object_smart_callback_add(cont, "expanded", _expand, NULL);
   evas_object_smart_callback_add(cont, "contracted", _contract, NULL);

   elm_genlist_item_append(cont, gitc, (void *)(uintptr_t)ITEM_MONTHS, NULL,
                                 ELM_GENLIST_ITEM_NONE, NULL, NULL);

   git = elm_genlist_item_append(cont, gitc, (void *)(uintptr_t)ITEM_DEBITS, NULL,
                                 ELM_GENLIST_ITEM_TREE, NULL, NULL);
   eo_key_data_set(git, "idesc_list", ydesc->debits);
   elm_genlist_item_expanded_set(git, EINA_TRUE);

   git = elm_genlist_item_append(cont, gitc, (void *)(uintptr_t)ITEM_SAVINGS, NULL,
                                 ELM_GENLIST_ITEM_TREE, NULL, NULL);
   eo_key_data_set(git, "idesc_list", ydesc->savings);
   elm_genlist_item_expanded_set(git, EINA_TRUE);

   git = elm_genlist_item_append(cont, gitc, (void *)(uintptr_t)ITEM_CREDITS, NULL,
                                 ELM_GENLIST_ITEM_TREE, NULL, NULL);
   eo_key_data_set(git, "idesc_list", ydesc->credits);
   elm_genlist_item_expanded_set(git, EINA_TRUE);

   return cont;
}

