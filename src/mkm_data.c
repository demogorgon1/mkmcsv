#include <string.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_data.h"
#include "mkm_error.h"

static void
mkm_data_set_column_string(
	mkm_data_column*				data,
	const char*						string_value)
{
	data->type = MKM_DATA_COLUMN_TYPE_STRING;

	mkm_strcpy(data->string_value, string_value, sizeof(data->string_value));
}

static void
mkm_data_set_column_integer(
	mkm_data_column*				data,
	uint32_t						integer_value)
{
	data->type = MKM_DATA_COLUMN_TYPE_INTEGER;
	data->integer_value = integer_value;
}

static void
mkm_data_set_column_bool(
	mkm_data_column*				data,
	mkm_bool						bool_value)
{
	data->type = MKM_DATA_COLUMN_TYPE_BOOL;
	data->bool_value = bool_value;
}

static void
mkm_data_process_column(
	const mkm_csv_row*				csv_row,
	sfc_card*						card,
	const mkm_config_column*		config,
	mkm_data_column*				data)
{
	switch(config->info->type)
	{
	case MKM_CONFIG_COLUMN_TYPE_CSV:
		mkm_data_set_column_integer(data, csv_row->columns[config->info->csv_column]);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_TCGPLAYER_ID:
		mkm_data_set_column_integer(data, card->data.tcgplayer_id);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_COLLECTOR_NUMBER:
		mkm_data_set_column_integer(data, card->data.collector_number);
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

	case MKM_CONFIG_COLUMN_TYPE_SFC_NAME:
		mkm_data_set_column_string(data, card->key.name);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_SET:
		mkm_data_set_column_string(data, card->key.set);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_VERSION:
		mkm_data_set_column_integer(data, card->key.version);
		break;

	case MKM_CONFIG_COLUMN_TYPE_SFC_STRING:					
		mkm_data_set_column_string(data, sfc_card_get_string(card, config->info->card_string));
		break;

	default:
		assert(0);
	}
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
	mkm_data*					data,
	const struct _mkm_csv*		csv)
{
	for(const mkm_csv_row* csv_row = csv->rows; csv_row != NULL; csv_row = csv_row->next)
	{
		/* Query card info */
		MKM_ERROR_CHECK(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_ID_PRODUCT), "No Cardmarket ID column.");

		sfc_query* query = sfc_cache_query_cardmarket_id(data->cache, csv_row->columns[MKM_CSV_COLUMN_ID_PRODUCT]);
		assert(query != NULL);
		MKM_ERROR_CHECK(sfc_query_wait(query, 10 * 1000) == SFC_RESULT_OK, "Failed waiting for query to finish.");
		MKM_ERROR_CHECK(query->result == SFC_RESULT_OK, "Query failed.");
		MKM_ERROR_CHECK(query->results->count == 1, "Got more results than expected.");
		sfc_card* card = query->results->cards[0];

		/* Allocate row and insert in linked list */
		mkm_data_row* row = MKM_NEW(mkm_data_row);
		if(data->last_row != NULL)
			data->last_row->next = row;
		else
			data->first_row = row;
		data->last_row = row;

		/* Generate row */
		row->columns = (mkm_data_column*)malloc(sizeof(mkm_data_column) * data->config->num_columns);
		assert(row->columns != NULL);
		size_t column_index = 0;

		for(const mkm_config_column* column = data->config->columns; column != NULL; column = column->next)
		{
			assert(column_index < data->config->num_columns);
			mkm_data_column* data_column = &row->columns[column_index++];

			mkm_data_process_column(csv_row, card, column, data_column);
		}

		/* Clean up query */
		sfc_query_delete(query);
	}
}
