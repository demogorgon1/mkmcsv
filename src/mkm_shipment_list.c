#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <sfc/sfc_collector_number.h>

#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_price.h"
#include "mkm_shipment.h"
#include "mkm_shipment_list.h"
#include "mkm_tokenize.h"

static uint32_t
mkm_shipment_list_parse_condition_string(
	const char*			string)
{
	size_t len = strlen(string);
	
	if(len == 2)
	{
		char lc_string[3];
		lc_string[0] = (char)tolower(string[0]);
		lc_string[1] = (char)tolower(string[1]);
		lc_string[2] = '\0';

		if(strcmp(lc_string, "po") == 0)
			return 7;
		if (strcmp(lc_string, "pl") == 0)
			return 6;
		if (strcmp(lc_string, "lp") == 0)
			return 5;
		if (strcmp(lc_string, "gd") == 0)
			return 4;
		if (strcmp(lc_string, "ex") == 0)
			return 3;
		if (strcmp(lc_string, "nm") == 0)
			return 2;
		if (strcmp(lc_string, "mt") == 0)
			return 1;
	}

	mkm_error("Invalid condition string: %s", string);
	return 0;
}

static void
mkm_shipment_list_make_card_key(
	sfc_card_key*		card_key,
	const char*			set,
	const char*			collector_number_string)
{
	size_t set_len = strlen(set);
	MKM_ERROR_CHECK(set_len < SFC_MAX_SET, "Set name too long: %s", set);
	memcpy(card_key->set, set, set_len + 1);

	sfc_result result = sfc_collector_number_from_string(
		collector_number_string, 
		&card_key->collector_number);

	MKM_ERROR_CHECK(result == SFC_RESULT_OK, "Failed to parse collector number string.");
}

static mkm_bool
mkm_shipment_list_is_whitespace(
	char				character)
{
	switch(character)
	{
	case '\n':
	case '\r':
	case '\t':
	case ' ':
		return MKM_TRUE;
	}
	return MKM_FALSE;
}

static void
mkm_shipment_list_trim_string(
	char*				string)
{
	size_t len = strlen(string);

	/* Count left whitespaces */
	size_t trim_left = 0;
	while(mkm_shipment_list_is_whitespace(string[trim_left]))
		trim_left++;

	/* Trim left */
	for(size_t i = 0; i < len - trim_left; i++)
		string[i] = string[i + trim_left];

	len -= trim_left;
	string[len] = '\0';

	if(len > 0)
	{
		/* Trim right */
		for (size_t i = len - 1; i > 0 && mkm_shipment_list_is_whitespace(string[i]); i--)
			string[i] = '\0';
	}
}

static uint32_t
mkm_shipment_list_parse_uint32(
	const char*			string)
{
	uint64_t v = (uint64_t)strtoull(string, NULL, 10);
	MKM_ERROR_CHECK(v <= UINT32_MAX, "Number too large.");
	return (uint32_t)v;
}

static void
mkm_shipment_list_make_csv_path(
	const char*			csv_template,
	uint32_t			purchase_id,
	char*				buffer,
	size_t				buffer_size)
{
	const char* p = csv_template;
	size_t i = 0;
	
	char purchase_id_string[32];
	int required = snprintf(purchase_id_string, sizeof(purchase_id_string), "%u", purchase_id);
	MKM_ERROR_CHECK(required <= (int)sizeof(purchase_id_string), "Buffer overflow.");
	size_t purchase_id_string_length = strlen(purchase_id_string);

	while(*p != '\0')
	{
		if(*p == '{')
		{
			MKM_ERROR_CHECK(p[1] == '}', "Expected '}' in CSV path template.");
			p++;

			MKM_ERROR_CHECK(i + purchase_id_string_length <= buffer_size, "Buffer overflow.");
			memcpy(&buffer[i], purchase_id_string, purchase_id_string_length);
			i += purchase_id_string_length;
		}
		else
		{
			MKM_ERROR_CHECK(i < buffer_size, "Buffer overflow.");
			buffer[i++] = *p;
		}

		p++;
	}

	MKM_ERROR_CHECK(i < buffer_size, "Buffer overflow.");
	buffer[i] = '\0';
}

/*-----------------------------------------------------------------------------*/

mkm_shipment_list* 
mkm_shipment_list_create_from_file(
	const char*			path)
{
	mkm_shipment_list* shipment_list = MKM_NEW(mkm_shipment_list);
	
	mkm_strcpy(shipment_list->csv_template, "ArticlesFromShipment{}..csv", sizeof(shipment_list->csv_template));

	FILE* f = fopen(path, "rb");
	MKM_ERROR_CHECK(f != NULL, "Failed to open file: %s", path);

	{
		char line_buffer[1024];
		uint32_t line_num = 1;

		mkm_shipment* shipment = NULL;

		while(fgets(line_buffer, sizeof(line_buffer), f) != NULL)
		{
			/* Remove comments */
			{
				size_t len = strlen(line_buffer);
				for(size_t i = 0; i < len; i++)
				{
					if(line_buffer[i] == ';')
					{
						line_buffer[i] = '\0';
						break;
					}
				}
			}

			/* Trim */
			mkm_shipment_list_trim_string(line_buffer);

			if(line_buffer[0] != '\0')
			{
				/* Tokenize and parse line */
				mkm_tokenize tokenize;
				mkm_tokenize_string(&tokenize, line_buffer);
				assert(tokenize.num_tokens > 0);
				const char* first_token = tokenize.tokens[0];

				if(strcmp(first_token, "abort") == 0)
				{
					/* Ignore the rest of the file */
					break;
				}
				else if (strcmp(first_token, "ignore_csv") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 1, "Syntax error: ignore_csv");
					MKM_ERROR_CHECK(shipment != NULL, "'ignore_csv' without shipment.");
					
					shipment->ignore_csv = MKM_TRUE;
				}
				else if (strcmp(first_token, "adjust") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 2, "Syntax error: adjust <overall price adjustment>");
					MKM_ERROR_CHECK(shipment != NULL, "'adjust' without shipment.");

					shipment->overall_price_adjustment = mkm_price_parse(tokenize.tokens[1]);
				}
				else if(strcmp(first_token, "csv") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 2, "Syntax error: csv <path template>");
					mkm_strcpy(shipment_list->csv_template, tokenize.tokens[1], sizeof(shipment_list->csv_template));
				}
				else if(strcmp(first_token, "shipment") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 5, "Syntax error: shipment <id> <date> <shipping cost> <trustee fee>");
					
					/* Allocate shipment and add to end of linked list */
					shipment = mkm_shipment_create();

					if(shipment_list->tail != NULL)
						shipment_list->tail->next = shipment;
					else
						shipment_list->head = shipment;

					shipment_list->tail = shipment;

					/* Set shipment values */
					shipment->id = mkm_shipment_list_parse_uint32(tokenize.tokens[1]);
					shipment->date = mkm_shipment_list_parse_uint32(tokenize.tokens[2]);
					shipment->shipping_cost = mkm_price_parse(tokenize.tokens[3]);
					shipment->trustee_fee = mkm_price_parse(tokenize.tokens[4]);

					/* Determine CSV path */
					mkm_shipment_list_make_csv_path(
						shipment_list->csv_template, 
						shipment->id, 
						shipment->csv_path, 
						sizeof(shipment->csv_path));
				}
				else if(strcmp(first_token, "remove") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 4, "Syntax error: remove <set> <collector number> <condition>");
					MKM_ERROR_CHECK(shipment != NULL, "'remove' without shipment.");

					sfc_card_key key;
					mkm_shipment_list_make_card_key(&key, tokenize.tokens[1], tokenize.tokens[2]);

					/* FIXME: support supplying price */
					mkm_shipment_remove(
						shipment, 
						&key, 
						mkm_shipment_list_parse_condition_string(tokenize.tokens[3]),
						UINT32_MAX);
				}
				else if(strcmp(first_token, "add") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 5, "Syntax error: add <set> <collect number> <condition> <price>");
					MKM_ERROR_CHECK(shipment != NULL, "'add' without shipment.");

					sfc_card_key key;
					mkm_shipment_list_make_card_key(&key, tokenize.tokens[1], tokenize.tokens[2]);

					mkm_shipment_add(
						shipment, 
						&key, 
						mkm_shipment_list_parse_condition_string(tokenize.tokens[3]),
						mkm_price_parse(tokenize.tokens[4]));
				}
				else
				{
					mkm_error("Unexpected token: %s", first_token);
				}
			}

			line_num++;
		}
	}

	fclose(f);

	return shipment_list;
}

void				
mkm_shipment_list_destroy(
	mkm_shipment_list*	shipment_list)
{
	/* Free shipments */
	{
		mkm_shipment* shipment = shipment_list->head;
		while (shipment != NULL)
		{
			mkm_shipment* next = shipment->next;
			mkm_shipment_destroy(shipment);
			shipment = next;
		}
	}

	free(shipment_list);
}
