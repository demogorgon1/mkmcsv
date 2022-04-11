#ifndef __MKM_PURCHASE_H__
#define __MKM_PURCHASE_H__

#include <sfc/sfc_card.h>

#include "mkm_base.h"

typedef struct _mkm_purchase_modification
{
	sfc_card_key						card_key;
	uint32_t							condition;
	uint32_t							price;
	struct _mkm_purchase_modification*	next;
} mkm_purchase_modification;

typedef struct _mkm_purchase_modification_list
{
	mkm_purchase_modification*			head;
	mkm_purchase_modification*			tail;
} mkm_purchase_modification_list;

typedef struct _mkm_purchase 
{
	uint32_t							id;
	uint32_t							date;
	uint32_t							shipping_cost;
	uint32_t							trustee_fee;
	uint32_t							overall_price_adjustment;

	mkm_purchase_modification_list		additions;
	mkm_purchase_modification_list		removals;

	mkm_bool							ignore_csv;

	char								csv_path[256];
	struct _mkm_csv*					csv;

	struct _mkm_purchase*				next;
} mkm_purchase;

mkm_purchase*	mkm_purchase_create();

void			mkm_purchase_destroy(
					mkm_purchase*		purchase);

void			mkm_purchase_add(
					mkm_purchase*		purchase,
					const sfc_card_key*	card_key,
					uint32_t			condition,
					uint32_t			price);

void			mkm_purchase_remove(
					mkm_purchase*		purchase,
					const sfc_card_key*	card_key,
					uint32_t			condition,
					uint32_t			price);

#endif /* __MKM_PURCHASE_H__ */