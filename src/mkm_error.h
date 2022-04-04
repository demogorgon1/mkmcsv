#ifndef __MKM_ERROR_H__
#define __MKM_ERROR_H__

#define MKM_ERROR_CHECK(_condition, ...)		\
	do											\
	{											\
		if(!((int)(_condition)))				\
			mkm_error(__VA_ARGS__);				\
	} while(0)

void	mkm_error(
			const char*		format,
			...);

#endif /* __MKM_ERROR_H__ */