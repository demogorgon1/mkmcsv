#include "mkm_csv.h"
#include "mkm_purchase.h"

mkm_purchase* 
mkm_purchase_create()
{
	return MKM_NEW(mkm_purchase);
}

void			
mkm_purchase_destroy(
	mkm_purchase*	purchase)
{
	if(purchase->csv != NULL)
		mkm_csv_destroy(purchase->csv);

	free(purchase);
}
