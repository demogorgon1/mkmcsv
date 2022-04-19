#ifndef __mkm_shipment_H__
#define __mkm_shipment_H__

#include <sfc/sfc_card.h>

#include "mkm_base.h"

typedef struct _mkm_shipment_modification
{
	sfc_card_key						card_key;
	uint32_t							condition;
	int32_t								price;
	uint32_t							extra_row_number;
	struct _mkm_shipment_modification*	next;
} mkm_shipment_modification;

typedef struct _mkm_shipment_modification_list
{
	mkm_shipment_modification*			head;
	mkm_shipment_modification*			tail;
} mkm_shipment_modification_list;

typedef struct _mkm_shipment 
{
	uint32_t							id;
	uint32_t							date;
	int32_t								shipping_cost;
	int32_t								trustee_fee;
	int32_t								overall_price_adjustment;

	mkm_shipment_modification_list		additions;
	mkm_shipment_modification_list		removals;

	mkm_bool							ignore_csv;

	char								csv_path[256];
	struct _mkm_csv*					csv;

	uint32_t							next_extra_row_number;

	struct _mkm_shipment*				next;
} mkm_shipment;

mkm_shipment*	mkm_shipment_create();

void			mkm_shipment_destroy(
					mkm_shipment*		shipment);

void			mkm_shipment_add(
					mkm_shipment*		shipment,
					const sfc_card_key*	card_key,
					uint32_t			condition,
					int32_t				price);

void			mkm_shipment_remove(
					mkm_shipment*		shipment,
					const sfc_card_key*	card_key,
					uint32_t			condition,
					int32_t				price);

#endif /* __mkm_shipment_H__ */