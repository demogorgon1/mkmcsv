#ifndef __MKMCSV_BASE_H__
#define __MKMCSV_BASE_H__

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define MKM_TRUE	1
#define MKM_FALSE	0

typedef int mkm_bool;

static void* 
mkm_zalloc(
	size_t			size)
{
	void* p = malloc(size);
	assert(p != NULL);
	memset(p, 0, size);
	return p;
}

#define MKM_NEW(_type) ((_type*)mkm_zalloc(sizeof(_type)))

#endif /* __MKMCSV_BASE_H__ */