#ifndef __MKMCSV_CONFIG_H__
#define __MKMCSV_CONFIG_H__

#include <stdio.h>

#include <sfc/sfc_card.h>

#include "mkm_base.h"

typedef enum _mkm_config_column_type
{
	MKM_CONFIG_COLUMN_TYPE_UNDEFINED,
	MKM_CONFIG_COLUMN_TYPE_CSV,
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
	MKM_CONFIG_COLUMN_TYPE_SFC_NAME,
	MKM_CONFIG_COLUMN_TYPE_SFC_SET,
	MKM_CONFIG_COLUMN_TYPE_SFC_VERSION,
	MKM_CONFIG_COLUMN_TYPE_SFC_STRING
} mkm_config_column_type;

typedef struct _mkm_config_column_info
{
	const char*							name;
	mkm_config_column_type				type;
	sfc_card_string						card_string;
	uint8_t								csv_column;
} mkm_config_column_info;

typedef struct _mkm_config_column
{
	const mkm_config_column_info*		info;
	struct _mkm_config_column*			next;
} mkm_config_column;

typedef struct _mkm_config_input_file
{
	const char*							path;
	struct _mkm_config_input_file*		next;
} mkm_config_input_file;

struct _mkm_data;

typedef void (*mkm_input_callback)(
	struct _mkm_data*					data);

typedef void (*mkm_output_callback)(
	struct _mkm_data*					data);

typedef struct _mkm_config
{
	uint32_t							flags;
	char								cache_file[256];
	mkm_config_input_file*				input_files;
	mkm_config_column*					columns;
	size_t								num_columns;
	mkm_input_callback					input_callback;
	mkm_output_callback					output_callback;
	FILE*								output_stream;
} mkm_config;

void		mkm_config_init(
				mkm_config*				config,
				int						argc,
				char**					argv);

void		mkm_config_uninit(
				mkm_config*				config);

#endif /* __MKMCSV_CONFIG_H__ */