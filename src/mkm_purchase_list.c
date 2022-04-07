#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_price.h"
#include "mkm_purchase.h"
#include "mkm_purchase_list.h"
#include "mkm_tokenize.h"

static uint32_t
mkm_purchase_list_parse_condition_string(
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
mkm_purchase_list_make_card_key(
	sfc_card_key*		card_key,
	const char*			set,
	const char*			name,
	uint32_t			version)
{
	MKM_ERROR_CHECK(version <= UINT8_MAX, "Invalid card version: %u", version);
	card_key->version = (uint8_t)version;

	size_t set_len = strlen(set);
	MKM_ERROR_CHECK(set_len < SFC_MAX_SET, "Set name too long: %s", set);
	memcpy(card_key->set, set, set_len + 1);

	size_t name_len = strlen(name);
	MKM_ERROR_CHECK(name_len < SFC_MAX_NAME, "Card name too long: %s", name);
	memcpy(card_key->name, name, name_len + 1);
}

static mkm_bool
mkm_purchase_list_is_whitespace(
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
mkm_purchase_list_trim_string(
	char*				string)
{
	size_t len = strlen(string);

	/* Count left whitespaces */
	size_t trim_left = 0;
	while(mkm_purchase_list_is_whitespace(string[trim_left]))
		trim_left++;

	/* Trim left */
	for(size_t i = 0; i < len - trim_left; i++)
		string[i] = string[i + trim_left];

	len -= trim_left;
	string[len] = '\0';

	if(len > 0)
	{
		/* Trim right */
		for (size_t i = len - 1; i > 0 && mkm_purchase_list_is_whitespace(string[i]); i--)
			string[i] = '\0';
	}
}

static uint32_t
mkm_purchase_list_parse_uint32(
	const char*			string)
{
	uint64_t v = (uint64_t)strtoull(string, NULL, 10);
	MKM_ERROR_CHECK(v <= UINT32_MAX, "Number too large.");
	return (uint32_t)v;
}

static void
mkm_purchase_list_make_csv_path(
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

mkm_purchase_list* 
mkm_purchase_list_create_from_file(
	const char*			path)
{
	mkm_purchase_list* purchase_list = MKM_NEW(mkm_purchase_list);
	
	FILE* f = fopen(path, "rb");
	MKM_ERROR_CHECK(f != NULL, "Failed to open file: %s", path);

	{
		char line_buffer[1024];
		uint32_t line_num = 1;

		mkm_purchase* purchase = NULL;

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
			mkm_purchase_list_trim_string(line_buffer);

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
				else if(strcmp(first_token, "csv") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 2, "Syntax error: cvs <path template>");
					mkm_strcpy(purchase_list->csv_template, tokenize.tokens[1], sizeof(purchase_list->csv_template));
				}
				else if(strcmp(first_token, "purchase") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 5, "Syntax error: purchase <id> <date> <shipping cost> <trustee fee>");
					
					/* Allocate purchase and add to end of linked list */
					purchase = mkm_purchase_create();

					if(purchase_list->tail != NULL)
						purchase_list->tail->next = purchase;
					else
						purchase_list->head = purchase;

					purchase_list->tail = purchase;

					/* Set purchase values */
					purchase->id = mkm_purchase_list_parse_uint32(tokenize.tokens[1]);
					purchase->date = mkm_purchase_list_parse_uint32(tokenize.tokens[2]);
					purchase->shipping_cost = mkm_price_parse(tokenize.tokens[3]);
					purchase->trustee_fee = mkm_price_parse(tokenize.tokens[4]);

					/* Load CSV */
					char csv_path[256];
					mkm_purchase_list_make_csv_path(purchase_list->csv_template, purchase->id, csv_path, sizeof(csv_path));
					purchase->csv = mkm_csv_create_from_file(csv_path);					
				}
				else if(strcmp(first_token, "remove") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 4, "Syntax error: remove <set> <condition> <name>");

					/* FIXME: support different card versions */
					sfc_card_key key;
					mkm_purchase_list_make_card_key(&key, tokenize.tokens[1], tokenize.tokens[3], 0);

					/* FIXME: support supplying price */
					mkm_purchase_remove(
						purchase, 
						&key, 
						mkm_purchase_list_parse_condition_string(tokenize.tokens[2]),
						UINT32_MAX);
				}
				else if(strcmp(first_token, "add") == 0)
				{
					MKM_ERROR_CHECK(tokenize.num_tokens == 5, "Syntax error: add <set> <condition> <name> <price>");

					/* FIXME: support different card versions */
					sfc_card_key key;
					mkm_purchase_list_make_card_key(&key, tokenize.tokens[1], tokenize.tokens[3], 0);

					mkm_purchase_add(
						purchase, 
						&key, 
						mkm_purchase_list_parse_condition_string(tokenize.tokens[2]),
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

	return purchase_list;
}

void				
mkm_purchase_list_destroy(
	mkm_purchase_list*	purchase_list)
{
	/* Free purchases */
	{
		mkm_purchase* purchase = purchase_list->head;
		while (purchase != NULL)
		{
			mkm_purchase* next = purchase->next;
			mkm_purchase_destroy(purchase);
			purchase = next;
		}
	}

	free(purchase_list);
}
