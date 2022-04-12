#include "mkm_csv.h"
#include "mkm_shipment.h"

static void
mkm_shipment_modification_list_add(
	mkm_shipment_modification_list*	list,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_shipment_modification* modification = MKM_NEW(mkm_shipment_modification);

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
mkm_shipment_modification_list_uninit(
	mkm_shipment_modification_list* list)
{
	mkm_shipment_modification* modification = list->head;
	while(modification != NULL)
	{
		mkm_shipment_modification* next = modification->next;
		free(modification);
		modification = next;
	}
}

/*----------------------------------------------------------------------*/

mkm_shipment* 
mkm_shipment_create()
{
	return MKM_NEW(mkm_shipment);
}

void			
mkm_shipment_destroy(
	mkm_shipment*					shipment)
{
	if(shipment->csv != NULL)
		mkm_csv_destroy(shipment->csv);

	mkm_shipment_modification_list_uninit(&shipment->additions);
	mkm_shipment_modification_list_uninit(&shipment->removals);

	free(shipment);
}

void			
mkm_shipment_add(
	mkm_shipment*					shipment,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_shipment_modification_list_add(&shipment->additions, card_key, condition, price);
}

void			
mkm_shipment_remove(
	mkm_shipment*					shipment,
	const sfc_card_key*				card_key,
	uint32_t						condition,
	int32_t							price)
{
	mkm_shipment_modification_list_add(&shipment->removals, card_key, condition, price);
}