#ifndef __MKM_DATA_H__
#define __MKM_DATA_H__

#include "mkm_base.h"

struct _mkm_csv;

typedef enum _mkm_data_column_type
{
	MKM_DATA_COLUMN_TYPE_STRING,
	MKM_DATA_COLUMN_TYPE_INTEGER,
	MKM_DATA_COLUMN_TYPE_BOOL
} mkm_data_column_type;

typedef struct _mkm_data_column
{
	mkm_data_column_type		type;

	/* FIXME: union */
	char						string_value[256];
	uint32_t					integer_value;
	mkm_bool					bool_value;
} mkm_data_column;

typedef struct _mkm_data_row
{
	mkm_data_column*			columns;
	struct _mkm_data_row*		next;
} mkm_data_row;

typedef struct _mkm_data
{
	const struct _mkm_config*	config;
	struct _sfc_cache*			cache;

	mkm_data_row*				first_row;
	mkm_data_row*				last_row;
} mkm_data;

mkm_data*		mkm_data_create(
					const struct _mkm_config*	config,
					struct _sfc_cache*			cache);

void			mkm_data_destroy(
					mkm_data*					data);

void			mkm_data_process_csv(
					mkm_data*					data,
					const struct _mkm_csv*		csv);

#endif /* __MKM_DATA_H__ */