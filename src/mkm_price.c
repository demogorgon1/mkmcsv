#include "mkm_error.h"
#include "mkm_price.h"

uint32_t	
mkm_price_parse(
	const char*		string)
{
	const char* p = string; 
	
	uint32_t before_period_value = 0;
	uint32_t after_period_value = 0;
	mkm_bool before_period = MKM_TRUE;

	while(*p != '\0')
	{
		if(*p == '.')
		{
			MKM_ERROR_CHECK(before_period, "Invalid price: Syntax error.");
			before_period = MKM_FALSE;
		}
		else if(*p >= '0' && *p <= '9')
		{
			if(before_period)
				before_period_value = (before_period_value * 10) + (uint32_t)(*p - '0');
			else 
				after_period_value = (after_period_value * 10) + (uint32_t)(*p - '0');
		}
		else
		{
			mkm_error("Invalid price: Syntax error.");
		}

		p++;
	}

	MKM_ERROR_CHECK(after_period_value <= 99, "Invalid price: Too many decimals.");

	return before_period_value * 100 + after_period_value;
}
