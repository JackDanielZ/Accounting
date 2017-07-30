#include "common.h"

List *
list_next(const List *list)
{
   if (list) return list->next;
   return NULL;
}

void *
list_data_get(const List *list)
{
   if (list) return list->data;
   return NULL;
}

unsigned int
list_count(const List *list)
{
   int count = 0;
   const List *itr;
   void *data;
   LIST_FOREACH(list, itr, data) count++;
   (void) data;
   return count;
}

List *
list_append(List *list, const void *data)
{
   List *l, *new_l;

   new_l = calloc(1, sizeof(*new_l));
   if (!new_l) return list;

   new_l->next = NULL;
   new_l->data = (void *)data;
   if (!list)
     {
        new_l->prev = NULL;
        return new_l;
     }

   l = list;
   while (l->next) l = l->next;
   l->next = new_l;
   new_l->prev = l;

   return list;
}

