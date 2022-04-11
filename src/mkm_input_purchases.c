#include <stddef.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_data.h"
#include "mkm_input_purchases.h"
#include "mkm_purchase.h"
#include "mkm_purchase_list.h"

typedef struct _mkm_input_purchases_key_column_indices
{
	uint32_t	name;
	uint32_t	version;
	uint32_t	set;
	uint32_t	condition;
} mkm_input_purchases_key_column_indices;

static mkm_data_row*
mkm_input_purchases_find_row_in_array(
	const mkm_input_purchases_key_column_indices*	key_column_indices,
	mkm_data_row_array*								data_row_array,
	const mkm_purchase_modification*				modification)
{
	/* FIXME: do some kind of fuzzy search so not all info needs to be supplied by the modification */
	for(size_t i = 0; i < data_row_array->num_rows; i++)
	{
		mkm_data_row* data_row = data_row_array->rows[i];

		if(data_row == NULL || data_row->removed)
			continue;

		mkm_data_column* name_column = &data_row->columns[key_column_indices->name];
		mkm_data_column* version_column = &data_row->columns[key_column_indices->version];
		mkm_data_column* set_column = &data_row->columns[key_column_indices->set];
		mkm_data_column* condition_column = &data_row->columns[key_column_indices->condition];

		assert(name_column->type == MKM_DATA_COLUMN_TYPE_STRING);
		assert(version_column->type == MKM_DATA_COLUMN_TYPE_UINT32);
		assert(set_column->type == MKM_DATA_COLUMN_TYPE_STRING);
		assert(condition_column->type == MKM_DATA_COLUMN_TYPE_UINT32);
		
		if(strcmp(modification->card_key.name, name_column->string_value) == 0 &&
			strcmp(modification->card_key.set, set_column->string_value) == 0 &&
			modification->card_key.version == version_column->uint32_value &&
			modification->condition == condition_column->uint32_value)
		{
			return data_row;
		}
	}

	return NULL;
}

static mkm_data_row*
mkm_input_purchases_add_row(
	const mkm_purchase*								purchase,
	mkm_data*										data,
	const mkm_purchase_modification*				modification)
{
	/* Query the card requested by the modification */
	sfc_card* card = NULL;

	{
		sfc_query* query = sfc_cache_query_card_key(data->cache, &modification->card_key);
		assert(query != NULL);

		sfc_result result = sfc_query_wait(query, 10 * 1000);

		MKM_ERROR_CHECK(result == SFC_RESULT_OK, "Failed waiting for query to complete (%d)", result);
		MKM_ERROR_CHECK(query->result == SFC_RESULT_OK, "Query failed (%d)", query->result);
		MKM_ERROR_CHECK(query->results->count == 1, "More than one card in query result.");

		card = query->results->cards[0];

		sfc_query_delete(query);
	}

	/* Create the new row */
	mkm_csv_row csv_row;
	csv_row.columns[MKM_CSV_COLUMN_ID_PRODUCT] = card->data.cardmarket_id;
	csv_row.columns[MKM_CSV_COLUMN_PRICE] = modification->price;
	csv_row.columns[MKM_CSV_COLUMN_CONDITION] = modification->condition;
		
	/* FIXME: this should be made available too, optionally. mkm_csv_row should really be supplied from a higher level */
	/*csv_row.columns[MKM_CSV_COLUMN_ID_LANGUAGE] =
	csv_row.columns[MKM_CSV_COLUMN_IS_FOIL] = 
	csv_row.columns[MKM_CSV_COLUMN_ID_LANGUAGE] = 
	csv_row.columns[MKM_CSV_COLUMN_IS_SIGNED] = 
	csv_row.columns[MKM_CSV_COLUMN_IS_ALTERED] = 
	csv_row.columns[MKM_CSV_COLUMN_IS_PLAYSET] =*/
		
	csv_row.flags = (1 << MKM_CSV_COLUMN_ID_PRODUCT)
		| (1 << MKM_CSV_COLUMN_PRICE)
		| (1 << MKM_CSV_COLUMN_CONDITION);

	mkm_data_purchase_info purchase_info;
	purchase_info.date = purchase->date;
	purchase_info.id = purchase->id;
	purchase_info.shipping_cost = purchase->shipping_cost;
	purchase_info.trustee_fee = purchase->trustee_fee;

	mkm_data_row* row = mkm_data_create_row(data);

	size_t column_index = 0;

	for (const mkm_config_column* column = data->config->columns; column != NULL; column = column->next)
	{
		assert(column_index < data->config->num_columns);
		mkm_data_column* data_column = &row->columns[column_index++];

		mkm_data_process_column(&csv_row, card, column, &purchase_info, data_column);
	}

	return row;
}

static void
mkm_input_purchases_adjust_overall_price(
	const mkm_config*								config,
	mkm_data_row_array*								data_row_array,
	const mkm_purchase*								purchase)
{
	/* Fetch column index of price */
	uint32_t price_column_index = mkm_config_get_column_index_by_name(config, "price");
	MKM_ERROR_CHECK(price_column_index != UINT32_MAX, "'price' column is required for processing purchases.");

	/* Calculate total price sum */
	int32_t price_sum = 0;
	mkm_data_row* highest_price_row = NULL;
	int32_t highest_price = 0;

	for (size_t i = 0; i < data_row_array->num_rows; i++)
	{
		mkm_data_row* row = data_row_array->rows[i];
		if (row == NULL || row->removed)
			continue;

		const mkm_data_column* price_column = &row->columns[price_column_index];
		assert(price_column->type == MKM_DATA_COLUMN_TYPE_PRICE);
		price_sum += price_column->price_value;

		if (highest_price_row == NULL || highest_price < price_column->price_value)
		{
			highest_price_row = row;
			highest_price = price_column->price_value;
		}
	}

	/* Adjust prices based on the part of total price sum */
	int32_t sum_of_adjustments = 0;

	for (size_t i = 0; i < data_row_array->num_rows; i++)
	{
		mkm_data_row* row = data_row_array->rows[i];
		if (row == NULL || row->removed)
			continue;

		mkm_data_column* price_column = &row->columns[price_column_index];

		int32_t adjustment = (purchase->overall_price_adjustment * price_column->price_value) / price_sum;

		price_column->price_value += adjustment;

		sum_of_adjustments += adjustment;
	}

	/* Add any rounding loss to the most expensive row */
	int32_t rounding_error = purchase->overall_price_adjustment - sum_of_adjustments;

	if (rounding_error != 0)
	{
		assert(highest_price_row != NULL);

		highest_price_row->columns[price_column_index].price_value += rounding_error;
	}
}

static void
mkm_input_purchases_adjust_shipping_costs_and_trustee_fees(
	const mkm_config*								config,
	mkm_data_row_array*								data_row_array,
	const mkm_purchase*								purchase)
{
	/* Fetch column indices we care about */
	uint32_t price_column_index = mkm_config_get_column_index_by_name(config, "price");
	MKM_ERROR_CHECK(price_column_index != UINT32_MAX, "'price' column is required for processing purchases.");
	uint32_t shipping_cost_column_index = mkm_config_get_column_index_by_name(config, "shipping_cost");
	MKM_ERROR_CHECK(shipping_cost_column_index != UINT32_MAX, "'shipping_cost' column is required for processing purchases.");
	uint32_t trustee_fee_column_index = mkm_config_get_column_index_by_name(config, "trustee_fee");
	MKM_ERROR_CHECK(trustee_fee_column_index != UINT32_MAX, "'trustee_fee' column is required for processing purchases.");

	/* Calculate total price sum */
	int32_t price_sum = 0;
	mkm_data_row* highest_price_row = NULL;
	int32_t highest_price = 0;

	for(size_t i = 0; i < data_row_array->num_rows; i++)
	{
		mkm_data_row* row = data_row_array->rows[i];
		if(row == NULL || row->removed)
			continue;

		const mkm_data_column* price_column = &row->columns[price_column_index];
		assert(price_column->type == MKM_DATA_COLUMN_TYPE_PRICE);
		price_sum += price_column->price_value;

		if(highest_price_row == NULL || highest_price < price_column->price_value)
		{
			highest_price_row = row;
			highest_price = price_column->price_value;
		}
	}

	/* Adjust shipping costs and trustee fees based on the part of total price sum */
	int32_t sum_of_shipping_costs = 0;
	int32_t sum_of_trustee_fees = 0;

	for (size_t i = 0; i < data_row_array->num_rows; i++)
	{
		mkm_data_row* row = data_row_array->rows[i];
		if(row == NULL || row->removed)
			continue;

		mkm_data_column* price_column = &row->columns[price_column_index];
		mkm_data_column* shipping_cost_column = &row->columns[shipping_cost_column_index];
		mkm_data_column* trustee_fee_column = &row->columns[trustee_fee_column_index];

		assert(shipping_cost_column->type == MKM_DATA_COLUMN_TYPE_PRICE);
		assert(trustee_fee_column->type == MKM_DATA_COLUMN_TYPE_PRICE);

		shipping_cost_column->price_value = (purchase->shipping_cost * price_column->price_value) / price_sum;
		trustee_fee_column->price_value = (purchase->trustee_fee * price_column->price_value) / price_sum;

		sum_of_shipping_costs += shipping_cost_column->price_value;
		sum_of_trustee_fees += trustee_fee_column->price_value;
	}

	/* Add any rounding loss to the most expensive row */
	assert(sum_of_shipping_costs <= purchase->shipping_cost);
	assert(sum_of_trustee_fees <= purchase->trustee_fee);

	int32_t shipping_cost_rounding_error = purchase->shipping_cost - sum_of_shipping_costs;
	int32_t trustee_fee_rounding_error = purchase->trustee_fee - sum_of_trustee_fees;

	if(shipping_cost_rounding_error > 0 || trustee_fee_rounding_error > 0)
	{
		assert(highest_price_row != NULL);

		highest_price_row->columns[shipping_cost_column_index].price_value += shipping_cost_rounding_error;
		highest_price_row->columns[trustee_fee_column_index].price_value += trustee_fee_rounding_error;
	}
}

/*--------------------------------------------------------------------*/

void	
mkm_input_purchases(
	struct _mkm_data*								data)
{
	mkm_input_purchases_key_column_indices key_column_indices;
	key_column_indices.name = mkm_config_get_column_index_by_name(data->config, "name");
	key_column_indices.version = mkm_config_get_column_index_by_name(data->config, "version");
	key_column_indices.set = mkm_config_get_column_index_by_name(data->config, "set");
	key_column_indices.condition = mkm_config_get_column_index_by_name(data->config, "condition");
	MKM_ERROR_CHECK(key_column_indices.name != UINT32_MAX, "'name' column is required for processing purchases.");
	MKM_ERROR_CHECK(key_column_indices.version != UINT32_MAX, "'version' column is required for processing purchases.");
	MKM_ERROR_CHECK(key_column_indices.set != UINT32_MAX, "'set' column is required for processing purchases.");
	MKM_ERROR_CHECK(key_column_indices.condition != UINT32_MAX, "'condition' column is required for processing purchases.");

	for(const mkm_config_input_file* input_file = data->config->input_files; input_file != NULL; input_file = input_file->next)
	{
		mkm_purchase_list* purchase_list = mkm_purchase_list_create_from_file(input_file->path);

		for(mkm_purchase* purchase = purchase_list->head; purchase != NULL; purchase = purchase->next)
		{
			printf("%u...\n", purchase->id);
			
			if(!purchase->ignore_csv)
			{
				/* Load CSV */
				purchase->csv = mkm_csv_create_from_file(purchase->csv_path);
			}

			/* Initialize of array of rows generated by this purchase */
			mkm_data_row_array data_row_array;
			mkm_data_row_array_init(&data_row_array, purchase->csv);

			/* Process CSV */
			if (purchase->csv != NULL)
			{
				mkm_data_purchase_info purchase_info;
				purchase_info.id = purchase->id;
				purchase_info.date = purchase->date;
				purchase_info.shipping_cost = purchase->shipping_cost;
				purchase_info.trustee_fee = purchase->trustee_fee;

				mkm_data_process_csv(data, purchase->csv, &purchase_info, &data_row_array);
			}

			/* Apply removals */
			for (const mkm_purchase_modification* modification = purchase->removals.head; modification != NULL; modification = modification->next)
			{
				mkm_data_row* row = mkm_input_purchases_find_row_in_array(&key_column_indices, &data_row_array, modification);
				MKM_ERROR_CHECK(row != NULL, "Row specified by removal operation not found: %s (%s, version %u)",
					modification->card_key.name, modification->card_key.set, modification->card_key.version);

				row->removed = MKM_TRUE;
			}

			/* Apply additions */
			for (const mkm_purchase_modification* modification = purchase->additions.head; modification != NULL; modification = modification->next)
			{
				mkm_data_row* row = mkm_input_purchases_add_row(purchase, data, modification);

				mkm_data_row_array_add(&data_row_array, row);
			}

			if(mkm_data_row_array_count_non_nulls(&data_row_array) > 0)
			{
				if(purchase->overall_price_adjustment != 0)
				{
					/* Adjust overall price of shipment */
					mkm_input_purchases_adjust_overall_price(data->config, &data_row_array, purchase);
				}
	
				/* Adjust shipping costs and trustee fees */
				mkm_input_purchases_adjust_shipping_costs_and_trustee_fees(data->config, &data_row_array, purchase);
			}

			mkm_data_row_array_uninit(&data_row_array);
		}

		mkm_purchase_list_destroy(purchase_list);
	}
}