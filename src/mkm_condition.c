#include <ctype.h>

#include "mkm_base.h"

typedef struct _mkm_condition_type
{
	const char*		string;
	uint32_t		condition;
} mkm_condition_type;

static mkm_condition_type g_mkm_condition_types[] =
{
	{ "po",	7 },
	{ "pl",	6 },
	{ "lp",	5 },
	{ "gd",	4 },
	{ "ex",	3 },
	{ "nm",	2 },
	{ "mt",	1 },

	{ NULL, 0 }
};

uint32_t	
mkm_condition_from_string(
	const char*		string)
{
	MKM_ERROR_CHECK(strlen(string) == 2, "Condition must be 2 characters long.");

	char lc_string[3];
	lc_string[0] = (char)tolower((int)string[0]);
	lc_string[1] = (char)tolower((int)string[1]);
	lc_string[2] = '\0';

	const mkm_condition_type* t = g_mkm_condition_types;
	while(t->string != NULL)
	{
		if(strcmp(t->string, lc_string) == 0)
			return t->condition;

		t++;
	}

	mkm_error("Invalid condition string: %s", string);
	return 0;
}

const char* 
mkm_condition_to_string(
	uint32_t		condition)
{
	const mkm_condition_type* t = g_mkm_condition_types;
	while (t->string != NULL)
	{
		if (t->condition == condition)
			return t->string;

		t++;
	}

	mkm_error("Invalid condition: %u", condition);
	return NULL;
}
