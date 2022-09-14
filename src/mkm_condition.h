#ifndef __MKM_CONDITION_H__
#define __MKM_CONDITION_H__

uint32_t	mkm_condition_from_string(
				const char*		string);
const char*	mkm_condition_to_string(
				uint32_t		condition);
const char*	mkm_condition_to_string_us(
				uint32_t		condition);

#endif /* __MKM_CONDITION_H__ */