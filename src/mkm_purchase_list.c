#include <stdio.h>

#include "mkm_error.h"
#include "mkm_purchase.h"
#include "mkm_purchase_list.h"
#include "mkm_tokenize.h"

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
				mkm_tokenize tokenize;
				mkm_tokenize_string(&tokenize, line_buffer);

				for(size_t i = 0; i < tokenize.num_tokens; i++)
				{
					printf("[%s]", tokenize.tokens[i]);
				}
				printf("\n");
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
