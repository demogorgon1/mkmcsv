#ifndef __MKM_DATA_H__
#define __MKM_DATA_H__

#include "mkm_base.h"

struct _sfc_card;
struct _mkm_csv;
struct _mkm_csv_row;

typedef enum _mkm_data_column_type
{
	MKM_DATA_COLUMN_TYPE_STRING,
	MKM_DATA_COLUMN_TYPE_UINT32,
	MKM_DATA_COLUMN_TYPE_PRICE,
	MKM_DATA_COLUMN_TYPE_BOOL
} mkm_data_column_type;

typedef struct _mkm_data_column
{
	mkm_data_column_type		type;

	/* FIXME: union */
	char						string_value[256];
	uint32_t					uint32_value;
	int32_t						price_value;
	mkm_bool					bool_value;
} mkm_data_column;

typedef struct _mkm_data_row
{
	mkm_bool					removed;
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

typedef struct _mkm_data_shipment_info
{
	uint32_t					id;
	uint32_t					date;
	int32_t						shipping_cost;
	int32_t						trustee_fee;
} mkm_data_shipment_info;

typedef struct _mkm_data_row_array
{
	size_t						num_rows;
	mkm_data_row**				rows;			
} mkm_data_row_array;

mkm_data*		mkm_data_create(
					const struct _mkm_config*		config,
					struct _sfc_cache*				cache);

void			mkm_data_destroy(
					mkm_data*						data);

void			mkm_data_process_csv(
					mkm_data*						data,
					const struct _mkm_csv*			csv,
					const mkm_data_shipment_info*	shipment_info,
					mkm_data_row_array*				out_row_array);

mkm_data_row*	mkm_data_create_row(
					mkm_data*						data);

void			mkm_data_process_column(
					const struct _mkm_csv_row*		csv_row,
					struct _sfc_card*				card,
					const mkm_config_column*		config,
					const mkm_data_shipment_info*	shipment_info,
					mkm_data_column*				data);

void			mkm_data_row_array_init(
					mkm_data_row_array*				row_array,
					const struct _mkm_csv*			csv);

void			mkm_data_row_array_uninit(
					mkm_data_row_array*				row_array);

void			mkm_data_row_array_add(
					mkm_data_row_array*				row_array,
					mkm_data_row*					row);

size_t			mkm_data_row_array_count_non_nulls(
					mkm_data_row_array*				row_array);

#endif /* __MKM_DATA_H__ */