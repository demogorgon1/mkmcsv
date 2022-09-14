#include <string.h>

#include <sfc/sfc.h>

#include "mkm_condition.h"
#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_data.h"
#include "mkm_error.h"
#include "mkm_price.h"

static void
mkm_data_row_debug_print(
	const mkm_config*				config,
	const mkm_data_row*				row)
{
	for(size_t i = 0; i < config->num_columns; i++)
	{
		const mkm_data_column* column = &row->columns[i];
		switch(column->type)
		{
		case MKM_DATA_COLUMN_TYPE_STRING:	printf("%s ", column->string_value); break;
		case MKM_DATA_COLUMN_TYPE_UINT32:	printf("%u ", column->uint32_value); break;
		case MKM_DATA_COLUMN_TYPE_PRICE:	printf("%d ", column->price_value); break;
		case MKM_DATA_COLUMN_TYPE_BOOL:		printf("%s ", column->bool_value ? "true" : "false"); break;
		default:							assert(0);
		}
	}

	printf("\n");
}

static void
mkm_data_set_column_string(
	mkm_data_column*				data,
	const char*						value)
{
	data->type = MKM_DATA_COLUMN_TYPE_STRING;

	/* We might lose data if it's too long */
	strncpy(data->string_value, value, sizeof(data->string_value));
}

static void
mkm_data_set_column_uint32(
	mkm_data_column*				data,
	uint32_t						value)
{
	data->type = MKM_DATA_COLUMN_TYPE_UINT32;
	data->uint32_value = value;
}

static void
mkm_data_set_column_price(
	mkm_data_column*				data,
	int32_t							value)
{
	data->type = MKM_DATA_COLUMN_TYPE_PRICE;
	data->price_value = value;
}

static void
mkm_data_set_column_bool(
	mkm_data_column*				data,
	mkm_bool						bool_value)
{
	data->type = MKM_DATA_COLUMN_TYPE_BOOL;
	data->bool_value = bool_value;
}

static int32_t
mkm_data_compare_rows(
	const mkm_config_sort_column*	sort_columns,
	const mkm_data_row*				a,
	const mkm_data_row*				b)
{
	for(const mkm_config_sort_column* sort_column = sort_columns; sort_column != NULL; sort_column = sort_column->next)
	{
		const mkm_data_column* column_a = &a->columns[sort_column->column_index];
		const mkm_data_column* column_b = &b->columns[sort_column->column_index];

		assert(column_a->type == column_b->type);

		int32_t result = 0;

		switch(column_a->type)
		{
		case MKM_DATA_COLUMN_TYPE_STRING:
			result = strcmp(column_b->string_value, column_a->string_value);
			break;
			
		case MKM_DATA_COLUMN_TYPE_UINT32:
			if(column_a->uint32_value != column_b->uint32_value)
				result = column_b->uint32_value > column_a->uint32_value ? 1 : -1;
			break;	

		case MKM_DATA_COLUMN_TYPE_PRICE:
			if (column_a->price_value != column_b->price_value)
				result = column_b->price_value > column_a->price_value ? 1 : -1;
			break;

		case MKM_DATA_COLUMN_TYPE_BOOL:
			if (column_a->bool_value != column_b->bool_value)
				result = column_b->bool_value ? 1 : -1;
			break;

		default:
			assert(0);
		}

		if(result != 0)
			return result * sort_column->order;
	}

	return 0;
}

static void
mkm_data_add_row_to_binary_search_tree(
	mkm_data*						data,
	mkm_data_row*					row)
{
	if(data->config->sort_columns == NULL)
		return;

	if(data->binary_search_tree_root == NULL)
	{
		data->binary_search_tree_root = row;
	}
	else
	{
		mkm_data_row* node = data->binary_search_tree_root;

		for(;;)
		{
			int32_t compare_result = mkm_data_compare_rows(data->config->sort_columns, node, row);

			if(compare_result >= 0)
			{
				if(node->right != NULL)
				{
					node = node->right;
				}
				else
				{
					node->right = row;
					break;
				}	
			}
			else
			{
				if (node->left != NULL)
				{
					node = node->left;
				}
				else
				{
					node->left = row;
					break;
				}
			}
		}
	}
}

void
mkm_data_tree_sort(
	mkm_data*					data,
	mkm_data_row*				row)
{
	if(row->left != NULL)
		mkm_data_tree_sort(data, row->left);

	row->next = NULL;

	if(data->last_row != NULL)
		data->last_row->next = row;
	else 
		data->first_row = row;

	data->last_row = row;

	if(row->right != NULL)
		mkm_data_tree_sort(data, row->right);
}

/*------------------------------------------------------------------*/

mkm_data* 
mkm_data_create(
	const struct _mkm_config*	config,
	struct _sfc_cache*			cache)
{
	mkm_data* data = MKM_NEW(mkm_data);
	data->config = config;
	data->cache = cache;
	return data;
}

void			
mkm_data_destroy(
	mkm_data*					data)
{
	/* Free rows */
	{
		mkm_data_row* row = data->first_row;
		while(row != NULL)
		{
			mkm_data_row* next = row->next;
			free(row->columns);
			free(row);
			row = next;
		}
	}

	free(data);
}

void			
mkm_data_process_csv(
	mkm_data*						data,
	const struct _mkm_csv*			csv,
	const mkm_data_shipment_info*	shipment_info,
	mkm_data_row_array*				out_row_array)
{
	size_t row_index = 0;

	for(const mkm_csv_row* csv_row = csv->rows; csv_row != NULL; csv_row = csv_row->next)
	{
		/* Query card info */
		MKM_ERROR_CHECK(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_ID_PRODUCT), "No Cardmarket ID column.");

		sfc_query* query = sfc_cache_query_cardmarket_id(data->cache, csv_row->columns[MKM_CSV_COLUMN_ID_PRODUCT]);
		assert(query != NULL);
		MKM_ERROR_CHECK(sfc_query_wait(query, 10 * 1000) == SFC_RESULT_OK, "Failed waiting for query to finish.");

		if(query->result == SFC_RESULT_NOT_FOUND)
		{
			/* We can't really know if it's an invalid cardmarket id or that it wasn't a card. It could
			   be something like a binder or sealed product. Just ignore it. */
		}
		else
		{
			MKM_ERROR_CHECK(query->result == SFC_RESULT_OK, "Query failed.");
			MKM_ERROR_CHECK(query->results->count == 1, "Got more results than expected.");
			sfc_card* card = query->results->cards[0];

			uint32_t count = 1;
			if (mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_GROUP_COUNT))
				count = csv_row->columns[MKM_CSV_COLUMN_GROUP_COUNT];

			uint32_t playset_card_count = 1;
			int32_t playset_total_price = 0;
			int32_t playset_single_price = 0;
			int32_t playset_single_price_rounding_error = 0;
			if(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_IS_PLAYSET) &&
				mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_PRICE))
			{
				if(csv_row->columns[MKM_CSV_COLUMN_IS_PLAYSET] != 0)
				{
					playset_card_count = 4;
					playset_total_price = (int32_t)csv_row->columns[MKM_CSV_COLUMN_PRICE];
					playset_single_price = playset_total_price / 4;
					playset_single_price_rounding_error = playset_total_price - playset_single_price * 4;
				}
			}

			for(uint32_t i = 0; i < count; i++)
			{
				for(uint32_t j = 0; j < playset_card_count; j++)
				{
					/* Allocate row and insert in linked list */
					mkm_data_row* row = mkm_data_create_row(data);

					/* If caller requested an array of rows, add it here */
					if (out_row_array != NULL)
					{
						assert(row_index < out_row_array->num_rows);
						out_row_array->rows[row_index] = row;
					}

					/* Generate row */
					size_t column_index = 0;
					for (const mkm_config_column* column = data->config->columns; column != NULL; column = column->next)
					{
						assert(column_index < data->config->num_columns);
						mkm_data_column* data_column = &row->columns[column_index++];

						mkm_data_process_column(csv_row, card, column, shipment_info, data_column);

						if(playset_card_count > 1 &&
							column->info->type == MKM_CONFIG_COLUMN_TYPE_CSV &&
							column->info->csv_column == MKM_CSV_COLUMN_PRICE)
						{
							assert(data_column->type == MKM_DATA_COLUMN_TYPE_PRICE);

							data_column->price_value = playset_single_price;

							/* Add rounding error to first card of playset */
							if(playset_single_price_rounding_error > 0 && j == 0)
								data_column->price_value += playset_single_price_rounding_error;
						}
					}

					/* Add it to binary search tree for sorting */
					mkm_data_add_row(data, row);

					row_index++;
				}
			}
		}

		/* Clean up query */
		sfc_query_delete(query);
	}
}

mkm_data_row* 
mkm_data_create_row(
	mkm_data*					data)
{
	mkm_data_row* row = MKM_NEW(mkm_data_row);

	row->columns = (mkm_data_column*)malloc(sizeof(mkm_data_column) * data->config->num_columns);
	assert(row->columns != NULL);
	memset(row->columns, 0, sizeof(mkm_data_column) * data->config->num_columns);

	if (data->last_row != NULL)
		data->last_row->next = row;
	else
		data->first_row = row;
	data->last_row = row;

	return row;
}

void
mkm_data_process_column(
	const struct _mkm_csv_row*		csv_row,
	struct _sfc_card*				card,
	const mkm_config_column*		config,
	const mkm_data_shipment_info*	shipment_info,
	mkm_data_column*				data)
{
	switch(config->info->type)
	{
	case MKM_CONFIG_COLUMN_TYPE_CSV:
		if(config->info->csv_column == MKM_CSV_COLUMN_PRICE)
			mkm_data_set_column_price(data, (int32_t)csv_row->columns[config->info->csv_column]);
		else
			mkm_data_set_column_uint32(data, csv_row->columns[config->info->csv_column]);
		break;

	case MKM_CONFIG_COLUMN_TYPE_CONDITION_STRING:
		MKM_ERROR_CHECK(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_CONDITION), "No condition information available.");
		mkm_data_set_column_string(data, mkm_condition_to_string(csv_row->columns[MKM_CSV_COLUMN_CONDITION]));
		break;

	case MKM_CONFIG_COLUMN_TYPE_CONDITION_STRING_US:
		MKM_ERROR_CHECK(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_CONDITION), "No condition information available.");
		mkm_data_set_column_string(data, mkm_condition_to_string_us(csv_row->columns[MKM_CSV_COLUMN_CONDITION]));
		break;

	case MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_ID:
		MKM_ERROR_CHECK(shipment_info != NULL, "No shipment information available.");
		mkm_data_set_column_uint32(data, shipment_info->id);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_DATE:
		MKM_ERROR_CHECK(shipment_info != NULL, "No shipment information available.");
		mkm_data_set_column_uint32(data, shipment_info->date);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SHIPMENT_SHIPPING_COST:
		MKM_ERROR_CHECK(shipment_info != NULL, "No shipment information available.");
		mkm_data_set_column_price(data, shipment_info->shipping_cost);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SHIPMENT_TRUSTEE_FEE:
		MKM_ERROR_CHECK(shipment_info != NULL, "No shipment information available.");
		mkm_data_set_column_price(data, shipment_info->trustee_fee);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SHIPMENT_UNIQUE_ID:
		MKM_ERROR_CHECK(shipment_info != NULL, "No shipment information available.");
		{
			/* purchase_id-<row #> uniquely identifies a purchased card */
			char temp[256];
			size_t required = (size_t)snprintf(temp, sizeof(temp), "%X-%u",
				shipment_info->id,
				csv_row->columns[MKM_CSV_COLUMN_ROW_NUMBER]);
			MKM_ERROR_CHECK(required <= sizeof(temp), "Buffer overflow.");

			mkm_data_set_column_string(data, temp);
		}
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_TCGPLAYER_ID:
		mkm_data_set_column_uint32(data, card->data.tcgplayer_id);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLLECTOR_NUMBER:
		mkm_data_set_column_uint32(data, SFC_COLLECTOR_NUMBER(card->key.collector_number));
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_RED:
		mkm_data_set_column_bool(data, card->data.colors & SFC_CARD_COLOR_RED);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLUE:
		mkm_data_set_column_bool(data, card->data.colors & SFC_CARD_COLOR_BLUE);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_GREEN:
		mkm_data_set_column_bool(data, card->data.colors & SFC_CARD_COLOR_GREEN);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLACK:
		mkm_data_set_column_bool(data, card->data.colors & SFC_CARD_COLOR_BLACK);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_WHITE:
		mkm_data_set_column_bool(data, card->data.colors & SFC_CARD_COLOR_WHITE);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_RED:
		mkm_data_set_column_bool(data, card->data.color_identity & SFC_CARD_COLOR_RED);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLUE:
		mkm_data_set_column_bool(data, card->data.color_identity & SFC_CARD_COLOR_BLUE);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_GREEN:
		mkm_data_set_column_bool(data, card->data.color_identity & SFC_CARD_COLOR_GREEN);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLACK:
		mkm_data_set_column_bool(data, card->data.color_identity & SFC_CARD_COLOR_BLACK);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_WHITE:
		mkm_data_set_column_bool(data, card->data.color_identity & SFC_CARD_COLOR_WHITE);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_SET:
		mkm_data_set_column_string(data, card->key.set);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_VERSION:
		mkm_data_set_column_uint32(data, SFC_COLLECTOR_NUMBER_VERSION(card->key.collector_number));
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_STRING:			
		{
			const char* string = sfc_card_get_string(card, config->info->card_string);
			if (config->info->card_string_is_price)
			{
				if(strcmp(string, "null") == 0)
					mkm_data_set_column_price(data, 0);
				else
					mkm_data_set_column_price(data, mkm_price_parse(string));
			}
			else
			{
				mkm_data_set_column_string(data, string);
			}
		}
		break;

	default:
		assert(0);
	}
}

void			
mkm_data_row_array_init(
	mkm_data_row_array*			row_array,
	const struct _mkm_csv*		csv)
{
	memset(row_array, 0, sizeof(mkm_data_row_array));

	if(csv != NULL)
	{
		for(const mkm_csv_row* csv_row = csv->rows; csv_row != NULL; csv_row = csv_row->next)
		{
			size_t produced_rows = 1;

			if(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_IS_PLAYSET) &&
				csv_row->columns[MKM_CSV_COLUMN_IS_PLAYSET] != 0)
			{
				produced_rows = 4;
			}

			if(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_GROUP_COUNT))
			{
				produced_rows *= csv_row->columns[MKM_CSV_COLUMN_GROUP_COUNT];
			}

			row_array->num_rows += produced_rows;
		}

		row_array->rows = (mkm_data_row**)malloc(sizeof(mkm_data_row*) * row_array->num_rows);
		assert(row_array->rows != NULL);
		memset(row_array->rows, 0, sizeof(mkm_data_row*) * row_array->num_rows);
	}
}

void			
mkm_data_row_array_uninit(
	mkm_data_row_array*			row_array)
{
	assert(row_array->rows != NULL);
	free(row_array->rows);
}

void			
mkm_data_row_array_add(
	mkm_data_row_array*			row_array,
	mkm_data_row*				row)
{
	row_array->num_rows++;
	row_array->rows = (mkm_data_row**)realloc(row_array->rows, sizeof(mkm_data_row*) * row_array->num_rows);
	assert(row_array->rows != NULL);
	row_array->rows[row_array->num_rows - 1] = row;
}

size_t			
mkm_data_row_array_count_non_nulls(
	mkm_data_row_array*			row_array)
{
	size_t count = 0;

	for(size_t i = 0; i < row_array->num_rows; i++)
	{
		if(row_array->rows[i] != NULL)
			count++;
	}

	return count;
}

void			
mkm_data_sort(
	mkm_data*					data)
{
	if(data->config->sort_columns == NULL)
		return;

	/* Rows are already organized in a binary search tree, so for sorting the linked list 
	   we just need to walk the tree */
	data->first_row = NULL;
	data->last_row = NULL;

	mkm_data_tree_sort(data, data->binary_search_tree_root);
}

void			
mkm_data_add_row(
	mkm_data*					data,
	mkm_data_row*				row)
{
	/* Insert in binary search tree */
	mkm_data_add_row_to_binary_search_tree(data, row);
}

mkm_bool		
mkm_data_row_should_output(
	mkm_data*					data,
	const mkm_data_row*			row)
{
	if(row->removed)
		return MKM_FALSE;

	if(data->config->set_filters != NULL)
	{
		uint32_t column_index = mkm_config_get_column_index_by_name(data->config, "set");
		MKM_ERROR_CHECK(column_index != UINT32_MAX, "'set' column required to filter by set.");

		const mkm_data_column* column = &row->columns[column_index];
		assert(column->type == MKM_DATA_COLUMN_TYPE_STRING);

		for (const mkm_config_set_filter* set_filter = data->config->set_filters; set_filter != NULL; set_filter = set_filter->next)
		{
			if (strcmp(set_filter->set, column->string_value) == 0)
			{
				if (data->config->flags & MKM_CONFIG_SET_FILTER_WHITELIST)
					return MKM_TRUE;
				else if (data->config->flags & MKM_CONFIG_SET_FILTER_BLACKLIST)
					return MKM_FALSE;
				else
					assert(0);
			}
		}

		if (data->config->flags & MKM_CONFIG_SET_FILTER_WHITELIST)
		{
			/* None of the sets in the whitelist matched */
			return MKM_FALSE;
		}
	}

	return MKM_TRUE;
}
