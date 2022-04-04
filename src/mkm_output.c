#include <string.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_output.h"

static void
mkm_output_set_column_string(
	mkm_output_column*				output,
	const char*						string_value)
{
	output->type = MKM_OUTPUT_COLUMN_TYPE_STRING;

	/* FIXME: warning if truncating string */
	strncpy(output->string_value, string_value, sizeof(output->string_value));
}

static void
mkm_output_set_column_integer(
	mkm_output_column*				output,
	uint32_t						integer_value)
{
	output->type = MKM_OUTPUT_COLUMN_TYPE_INTEGER;
	output->integer_value = integer_value;
}

static void
mkm_output_set_column_bool(
	mkm_output_column*				output,
	mkm_bool						bool_value)
{
	output->type = MKM_OUTPUT_COLUMN_TYPE_BOOL;
	output->bool_value = bool_value;
}

static void
mkm_output_process_column(
	const mkm_csv_row*				csv_row,
	sfc_card*						card,
	const mkm_config_output_column*	config,
	mkm_output_column*				output)
{
	switch(config->info->type)
	{
	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_CSV:
		mkm_output_set_column_integer(output, csv_row->columns[config->info->csv_column]);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_TCGPLAYER_ID:
		mkm_output_set_column_integer(output, card->data.tcgplayer_id);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLLECTOR_NUMBER:
		mkm_output_set_column_integer(output, card->data.collector_number);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IS_RED:
		mkm_output_set_column_bool(output, card->data.colors & SFC_CARD_COLOR_RED);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IS_BLUE:
		mkm_output_set_column_bool(output, card->data.colors & SFC_CARD_COLOR_BLUE);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IS_GREEN:
		mkm_output_set_column_bool(output, card->data.colors & SFC_CARD_COLOR_GREEN);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IS_BLACK:
		mkm_output_set_column_bool(output, card->data.colors & SFC_CARD_COLOR_BLACK);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IS_WHITE:
		mkm_output_set_column_bool(output, card->data.colors & SFC_CARD_COLOR_WHITE);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_RED:
		mkm_output_set_column_bool(output, card->data.color_identity & SFC_CARD_COLOR_RED);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLUE:
		mkm_output_set_column_bool(output, card->data.color_identity & SFC_CARD_COLOR_BLUE);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_GREEN:
		mkm_output_set_column_bool(output, card->data.color_identity & SFC_CARD_COLOR_GREEN);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLACK:
		mkm_output_set_column_bool(output, card->data.color_identity & SFC_CARD_COLOR_BLACK);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_WHITE:
		mkm_output_set_column_bool(output, card->data.color_identity & SFC_CARD_COLOR_WHITE);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_NAME:
		mkm_output_set_column_string(output, card->key.name);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_SET:
		mkm_output_set_column_string(output, card->key.set);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_VERSION:
		mkm_output_set_column_integer(output, card->key.version);
		break;

	case MKM_CONFIG_OUTPUT_COLUMN_TYPE_SFC_STRING:					
		mkm_output_set_column_string(output, sfc_card_get_string(card, config->info->card_string));
		break;

	default:
		assert(0);
	}
}

/*------------------------------------------------------------------*/

mkm_output* 
mkm_output_create(
	const struct _mkm_config*	config,
	struct _sfc_cache*			cache)
{
	mkm_output* output = MKM_NEW(mkm_output);
	output->config = config;
	output->cache = cache;
	return output;
}

void			
mkm_output_destroy(
	mkm_output*					output)
{
	/* Free output rows */
	{
		mkm_output_row* row = output->first_row;
		while(row != NULL)
		{
			mkm_output_row* next = row->next;
			free(row->columns);
			free(row);
			row = next;
		}
	}

	free(output);
}

void			
mkm_output_process_csv(
	mkm_output*					output,
	const struct _mkm_csv*		csv)
{
	for(const mkm_csv_row* csv_row = csv->rows; csv_row != NULL; csv_row = csv_row->next)
	{
		/* Query card info */
		MKM_ERROR_CHECK(mkm_csv_row_has_column(csv_row, MKM_CSV_COLUMN_ID_PRODUCT), "No Cardmarket ID column.");

		sfc_query* query = sfc_cache_query_cardmarket_id(output->cache, csv_row->columns[MKM_CSV_COLUMN_ID_PRODUCT]);
		assert(query != NULL);
		MKM_ERROR_CHECK(sfc_query_wait(query, 10 * 1000) == SFC_RESULT_OK, "Failed waiting for query to finish.");
		MKM_ERROR_CHECK(query->result == SFC_RESULT_OK, "Query failed.");
		MKM_ERROR_CHECK(query->results->count == 1, "Got more results than expected.");
		sfc_card* card = query->results->cards[0];

		/* Allocate output row and insert in linked list */
		mkm_output_row* row = MKM_NEW(mkm_output_row);
		if(output->last_row != NULL)
			output->last_row->next = row;
		else
			output->first_row = row;
		output->last_row = row;

		/* Generate output row */
		row->columns = (mkm_output_column*)malloc(sizeof(mkm_output_column) * output->config->num_output_columns);
		assert(row->columns != NULL);
		size_t column_index = 0;

		for(const mkm_config_output_column* column = output->config->output_columns; column != NULL; column = column->next)
		{
			assert(column_index < output->config->num_output_columns);
			mkm_output_column* output_column = &row->columns[column_index++];

			mkm_output_process_column(csv_row, card, column, output_column);
		}

		/* Clean up query */
		sfc_query_delete(query);
	}
}
