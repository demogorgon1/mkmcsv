#ifndef __MKM_PURCHASE_LIST_H__
#define __MKM_PURCHASE_LIST_H__

#include "mkm_base.h"

typedef struct _mkm_purchase_list
{
	char					csv_template[256];

	struct _mkm_purchase*	head;
	struct _mkm_purchase*	tail;
} mkm_purchase_list;

mkm_purchase_list*	mkm_purchase_list_create_from_file(
						const char*			path);

void				mkm_purchase_list_destroy(
						mkm_purchase_list*	purchase_list);

#endif /* __MKM_PURCHASE_LIST_H__ */