#ifndef __LIST_H__
#define __LIST_H__

typedef struct _List List;

struct _List
{
   void            *data; /**< Pointer to list element payload */
   List            *next; /**< Next member in the list */
   List            *prev; /**< Previous member in the list */
};

List *list_next(const List *list);
void *list_data_get(const List *list);
unsigned int list_count(const List *list);
List *list_append(List *list, const void *data);

#define LIST_FOREACH(list, l, _data)\
  for (l = list, _data = list_data_get(l); l; l = list_next(l), _data = list_data_get(l))

#endif
