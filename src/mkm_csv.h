#ifndef __MKM_CSV_H__
#define __MKM_CSV_H__

#include "mkm_base.h"

enum mkm_csv_column
{
	MKM_CSV_COLUMN_ID_PRODUCT,
	MKM_CSV_COLUMN_PRICE,
	MKM_CSV_COLUMN_ID_LANGUAGE,
	MKM_CSV_COLUMN_CONDITION,
	MKM_CSV_COLUMN_IS_FOIL,
	MKM_CSV_COLUMN_IS_SIGNED,
	MKM_CSV_COLUMN_IS_ALTERED,
	MKM_CSV_COLUMN_IS_PLAYSET,
	MKM_CSV_COLUMN_GROUP_COUNT,

	MKM_CSV_COLUMN_UNUSED,

	NUM_MKM_CSV_COLUMNS
};

typedef struct _mkm_csv_row
{
	uint32_t				flags;
	uint32_t				columns[NUM_MKM_CSV_COLUMNS];
	struct _mkm_csv_row*	next;
} mkm_csv_row;

typedef struct _mkm_csv
{
	size_t					num_rows;
	mkm_csv_row*			rows;
} mkm_csv;

mkm_csv*	mkm_csv_create_from_file(
				const char*			path);
void		mkm_csv_destroy(
				mkm_csv*			csv);
mkm_bool	mkm_csv_row_has_column(
				const mkm_csv_row*	row,
				uint32_t			index);

#endif /* __MKM_CSV_H__ */