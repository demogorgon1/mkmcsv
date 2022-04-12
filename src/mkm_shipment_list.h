#ifndef __mkm_shipment_LIST_H__
#define __mkm_shipment_LIST_H__

#include "mkm_base.h"

typedef struct _mkm_shipment_list
{
	char					csv_template[256];

	struct _mkm_shipment*	head;
	struct _mkm_shipment*	tail;
} mkm_shipment_list;

mkm_shipment_list*	mkm_shipment_list_create_from_file(
						const char*			path);

void				mkm_shipment_list_destroy(
						mkm_shipment_list*	shipment_list);

#endif /* __mkm_shipment_LIST_H__ */