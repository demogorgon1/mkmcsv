#ifndef __MKM_PURCHASE_H__
#define __MKM_PURCHASE_H__

#include "mkm_base.h"

typedef struct _mkm_purchase 
{
	struct _mkm_csv*		csv;
	struct _mkm_purchase*	next;
} mkm_purchase;

mkm_purchase*	mkm_purchase_create();

void			mkm_purchase_destroy(
					mkm_purchase*	purchase);

#endif /* __MKM_PURCHASE_H__ */