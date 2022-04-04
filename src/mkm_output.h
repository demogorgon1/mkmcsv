#ifndef __MKM_OUTPUT_H__
#define __MKM_OUTPUT_H__

#include "mkm_base.h"

struct _mkm_csv;

typedef enum _mkm_output_column_type
{
	MKM_OUTPUT_COLUMN_TYPE_STRING,
	MKM_OUTPUT_COLUMN_TYPE_INTEGER,
	MKM_OUTPUT_COLUMN_TYPE_BOOL
} mkm_output_column_type;

typedef struct _mkm_output_column
{
	mkm_output_column_type		type;

	/* FIXME: union */
	char						string_value[256];
	uint32_t					integer_value;
	mkm_bool					bool_value;
} mkm_output_column;

typedef struct _mkm_output_row
{
	mkm_output_column*			columns;
	struct _mkm_output_row*		next;
} mkm_output_row;

typedef struct _mkm_output
{
	const struct _mkm_config*	config;
	struct _sfc_cache*			cache;

	mkm_output_row*				first_row;
	mkm_output_row*				last_row;
} mkm_output;

mkm_output*		mkm_output_create(
					const struct _mkm_config*	config,
					struct _sfc_cache*			cache);

void			mkm_output_destroy(
					mkm_output*					output);

void			mkm_output_process_csv(
					mkm_output*					output,
					const struct _mkm_csv*		csv);

#endif /* __MKM_OUTPUT_H__ */