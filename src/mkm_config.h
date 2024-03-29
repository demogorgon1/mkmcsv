#ifndef __MKMCSV_CONFIG_H__
#define __MKMCSV_CONFIG_H__

#include <stdio.h>

#include <sfc/sfc_card.h>

#include "mkm_base.h"

#define	MKM_CONFIG_VERBOSE						0x00000001
#define MKM_CONFIG_CLOSE_OUTPUT_STREAM_ON_EXIT	0x00000002
#define MKM_CONFIG_SET_FILTER_WHITELIST			0x00000004
#define MKM_CONFIG_SET_FILTER_BLACKLIST			0x00000008

typedef enum _mkm_config_column_type
{
	MKM_CONFIG_COLUMN_TYPE_UNDEFINED,
	MKM_CONFIG_COLUMN_TYPE_CSV,
	MKM_CONFIG_COLUMN_TYPE_CSV_LANGUAGE_STRING,
	MKM_CONFIG_COLUMN_TYPE_CONDITION_STRING,
	MKM_CONFIG_COLUMN_TYPE_CONDITION_STRING_US,
	MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_ID,
	MKM_CONFIG_COLUMN_TYPE_SHIPMENT_PURCHASE_DATE,
	MKM_CONFIG_COLUMN_TYPE_SHIPMENT_SHIPPING_COST,
	MKM_CONFIG_COLUMN_TYPE_SHIPMENT_TRUSTEE_FEE,
	MKM_CONFIG_COLUMN_TYPE_SHIPMENT_UNIQUE_ID,
	MKM_CONFIG_COLUMN_TYPE_SFC_TCGPLAYER_ID,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLLECTOR_NUMBER,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_RED,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLUE,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_GREEN,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_BLACK,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IS_WHITE,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_RED,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLUE,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_GREEN,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_BLACK,
	MKM_CONFIG_COLUMN_TYPE_SFC_COLOR_IDENTITY_IS_WHITE,
	MKM_CONFIG_COLUMN_TYPE_SFC_SET,
	MKM_CONFIG_COLUMN_TYPE_SFC_VERSION,
	MKM_CONFIG_COLUMN_TYPE_SFC_STRING
} mkm_config_column_type;

typedef enum _mkm_config_column_data_type
{
	MKM_CONFIG_COLUMN_DATA_TYPE_UINT32,
	MKM_CONFIG_COLUMN_DATA_TYPE_PRICE,
	MKM_CONFIG_COLUMN_DATA_TYPE_BOOL,
	MKM_CONFIG_COLUMN_DATA_TYPE_STRING
} mkm_config_column_data_type;

typedef struct _mkm_config_column_info
{
	const char*							name;
	mkm_config_column_type				type;
	mkm_config_column_data_type			data_type;
	sfc_card_string						card_string;
	mkm_bool							card_string_is_price;
	uint8_t								csv_column;
} mkm_config_column_info;

typedef struct _mkm_config_column
{
	const mkm_config_column_info*		info;
	mkm_bool							hidden;
	struct _mkm_config_column*			next;
} mkm_config_column;

typedef struct _mkm_config_input_file
{
	char*								path;
	struct _mkm_config_input_file*		next;
} mkm_config_input_file;

struct _mkm_data;

typedef void (*mkm_config_input_callback)(
	struct _mkm_data*					data);

typedef void (*mkm_config_output_callback)(
	struct _mkm_data*					data);

typedef struct _mkm_config_sort_column
{
	uint32_t							column_index;
	int32_t								order;
	struct _mkm_config_sort_column*		next;
} mkm_config_sort_column;

typedef struct _mkm_config_set_filter
{
	char*								set;
	struct _mkm_config_set_filter*		next;
} mkm_config_set_filter;

typedef struct _mkm_config
{
	uint32_t							flags;
	char								cache_file[256];
	mkm_config_input_file*				input_files;
	mkm_config_column*					columns;
	size_t								num_columns;
	mkm_config_sort_column*				sort_columns;
	mkm_config_set_filter*				set_filters;
	mkm_config_input_callback			input_callback;
	mkm_config_output_callback			output_callback;
	FILE*								output_stream;

	/* SQL options */
	char								sql_table[256];
} mkm_config;

void		mkm_config_init(
				mkm_config*				config,
				int						argc,
				char**					argv);

void		mkm_config_uninit(
				mkm_config*				config);

uint32_t	mkm_config_get_column_index_by_name(
				const mkm_config*		config,
				const char*				name);

#endif /* __MKMCSV_CONFIG_H__ */