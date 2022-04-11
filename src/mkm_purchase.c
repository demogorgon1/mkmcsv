#include "mkm_csv.h"
#include "mkm_purchase.h"

static void
mkm_purchase_modification_list_add(
	mkm_purchase_modification_list*	list,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_purchase_modification* modification = MKM_NEW(mkm_purchase_modification);

	if(list->tail != NULL)
		list->tail->next = modification;
	else 
		list->head = modification;

	list->tail = modification;

	memcpy(&modification->card_key, card_key, sizeof(sfc_card_key));
	modification->condition = condition;
	modification->price = price;
}

static void
mkm_purchase_modification_list_uninit(
	mkm_purchase_modification_list* list)
{
	mkm_purchase_modification* modification = list->head;
	while(modification != NULL)
	{
		mkm_purchase_modification* next = modification->next;
		free(modification);
		modification = next;
	}
}

/*----------------------------------------------------------------------*/

mkm_purchase* 
mkm_purchase_create()
{
	return MKM_NEW(mkm_purchase);
}

void			
mkm_purchase_destroy(
	mkm_purchase*					purchase)
{
	if(purchase->csv != NULL)
		mkm_csv_destroy(purchase->csv);

	mkm_purchase_modification_list_uninit(&purchase->additions);
	mkm_purchase_modification_list_uninit(&purchase->removals);

	free(purchase);
}

void			
mkm_purchase_add(
	mkm_purchase*					purchase,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_purchase_modification_list_add(&purchase->additions, card_key, condition, price);
}

void			
mkm_purchase_remove(
	mkm_purchase*					purchase,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_purchase_modification_list_add(&purchase->removals, card_key, condition, price);
}