#ifndef __MKMCSV_BASE_H__
#define __MKMCSV_BASE_H__

#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#include "mkm_error.h"

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

static void
mkm_strcpy(
	char*			destination,
	const char*		source,
	size_t			destination_size)
{
	size_t source_size = strlen(source) + 1;
	MKM_ERROR_CHECK(source_size <= destination_size, "Buffer overflow.");
	memcpy(destination, source, source_size);
}

#define MKM_NEW(_type) ((_type*)mkm_zalloc(sizeof(_type)))
#define MKM_UNUSED(_x) ((void)_x)

#endif /* __MKMCSV_BASE_H__ */