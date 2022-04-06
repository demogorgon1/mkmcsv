#include "mkm_config.h"
#include "mkm_data.h"
#include "mkm_input_purchases.h"
#include "mkm_purchase.h"
#include "mkm_purchase_list.h"

void	
mkm_input_purchases(
	struct _mkm_data*	data)
{
	for(const mkm_config_input_file* input_file = data->config->input_files; input_file != NULL; input_file = input_file->next)
	{
		mkm_purchase_list* purchase_list = mkm_purchase_list_create_from_file(input_file->path);

		mkm_purchase_list_destroy(purchase_list);
	}
}