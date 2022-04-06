#include "mkm_base.h"
#include "mkm_error.h"
#include "mkm_tokenize.h"

static mkm_bool
mkm_tokenize_is_whitespace(
	char			character)
{
	switch(character)
	{
	case ' ':
	case '\n':
	case '\t':
	case '\r':
		return MKM_TRUE;
	}

	return MKM_FALSE;
}

static void
mkm_tokenize_add_token(
	mkm_tokenize*	tokenize,
	char*			string,
	size_t			token_start,
	size_t			token_end,
	mkm_bool		resolve_escape_codes)
{
	MKM_ERROR_CHECK(tokenize->num_tokens < MKM_TOKENIZE_MAX, "Too many tokens in string.");

	char* p = &string[token_start];
	tokenize->tokens[tokenize->num_tokens++] = p;
	string[token_end] = '\0';

	if(resolve_escape_codes)
	{
		/* Resolve escape codes */
		while(*p != '\0')
		{
			if(*p == '\\')
			{
				char c = p[1];
				char special = 0; 

				switch (c)
				{
				case 'n':	special = '\n'; break;
				case 'r':	special = '\r'; break;
				case 't':	special = '\t'; break;
				case '\\':	special = '\\'; break;
				case '\"':	special = '\"'; break;
				default:	assert(0); /* Already verified escape code earlier */
				}

				*p = special;

				/* Shift string once to the left */
				char* p_shift = p + 1;
				while(*p_shift != '\0')
				{
					p_shift[0] = p_shift[1];
					p_shift++;
				}
			}

			p++;
		}
	}
}

/*------------------------------------------------------------*/

void	
mkm_tokenize_string(
	mkm_tokenize*	tokenize,
	char*			string)
{
	memset(tokenize, 0, sizeof(mkm_tokenize));

	size_t i = 0;

	enum 
	{
		INIT,
		STRING,
		STRING_ESCAPE_CODE,
		TOKEN,
		END
	} state;

	state = INIT;
	size_t token_start = 0;

	while(state != END)
	{
		char c = string[i];

		switch(state)
		{
		case INIT:
			if(c == '\0')
			{
				state = END;
			}
			else if (mkm_tokenize_is_whitespace(c))
			{
				/* Ignore */
			}
			else if(c == '\"')
			{
				state = STRING;
				token_start = i + 1;
			}
			else
			{
				state = TOKEN;
				token_start = i;
			}
			break;

		case STRING:
			if(c == '\"')
			{
				mkm_tokenize_add_token(tokenize, string, token_start, i, MKM_TRUE);
				
				state = INIT;
			}
			else if(c == '\\')
			{
				state = STRING_ESCAPE_CODE;
			}
			else 
			{
				MKM_ERROR_CHECK(c != '\0', "Missing '\"'.");
			}
			break;

		case STRING_ESCAPE_CODE:
			MKM_ERROR_CHECK(c == '\"' || c == 't' || c == 'n' || c == 'r' || c == '\\', "Invalid escape code: %c", c);			
			state = STRING;
			break;

		case TOKEN:
			if(c == '\"')
			{
				mkm_tokenize_add_token(tokenize, string, token_start, i, MKM_FALSE);

				state = STRING;
				token_start = i + 1;
			}
			else if(mkm_tokenize_is_whitespace(c))
			{
				mkm_tokenize_add_token(tokenize, string, token_start, i, MKM_FALSE);

				state = INIT;
			}
			else if(c == '\0')
			{
				mkm_tokenize_add_token(tokenize, string, token_start, i, MKM_FALSE);

				state = END;
			}
			break;

		default:
			assert(0);
		}

		i++;
	}
}