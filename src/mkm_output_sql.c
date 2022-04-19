#include "mkm_config.h"
#include "mkm_data.h"
#include "mkm_output_sql.h"

static const char*
mkm_output_sql_get_table_name(
	const struct _mkm_config*	config)
{
	if(config->sql_table[0] != '\0')
		return config->sql_table;

	return "cards";
}

static mkm_bool
mkm_output_sql_is_last_visible_column(
	const mkm_config_column*	column)
{
	for(const mkm_config_column* i = column->next; i != NULL; i = i->next)
	{
		if(!i->hidden && strcmp(i->info->name, "unique_id") != 0)
			return MKM_FALSE;
	}

	return MKM_TRUE;
}

/*---------------------------------------------------------------*/

void	
mkm_output_sql(
	struct _mkm_data*			data)
{
	const char* table_name = mkm_output_sql_get_table_name(data->config);

	/* Create table if it doesn't already exist */
	{
		fprintf(data->config->output_stream, "CREATE TABLE IF NOT EXISTS %s (\n", table_name);
		fprintf(data->config->output_stream, "unique_id VARCHAR(64) PRIMARY KEY");

		for (const mkm_config_column* column = data->config->columns; column != NULL; column = column->next)
		{
			if (column->hidden)
				continue;

			if (strcmp(column->info->name, "unique_id") != 0)
			{
				fprintf(data->config->output_stream, ",\n%s ", column->info->name);

				const char* sql_type = NULL;

				switch (column->info->data_type)
				{
				case MKM_CONFIG_COLUMN_DATA_TYPE_BOOL:		sql_type = "SMALLINT"; break;
				case MKM_CONFIG_COLUMN_DATA_TYPE_PRICE:		sql_type = "INT"; break;
				case MKM_CONFIG_COLUMN_DATA_TYPE_STRING:	sql_type = "TEXT"; break;
				case MKM_CONFIG_COLUMN_DATA_TYPE_UINT32:	sql_type = "BIGINT"; break;
				default:									assert(0);
				}

				fprintf(data->config->output_stream, "%s ", sql_type);
			}
		}

		fprintf(data->config->output_stream, ");\n");
	}

	/* Insert rows */
	uint32_t unique_id_column_index = mkm_config_get_column_index_by_name(data->config, "unique_id");
	MKM_ERROR_CHECK(unique_id_column_index != UINT32_MAX, "'unique_id' column required for SQL output.");

	for (const mkm_data_row* row = data->first_row; row != NULL; row = row->next)
	{
		if (!mkm_data_row_should_output(data, row))
			continue;

		const mkm_data_column* unique_id_column = &row->columns[unique_id_column_index];
		MKM_ERROR_CHECK(unique_id_column->type == MKM_DATA_COLUMN_TYPE_STRING, "'unique_id' not a string.");

		fprintf(data->config->output_stream, "INSERT INTO %s (unique_id", table_name);

		uint32_t column_index = 0;

		for (const mkm_config_column* config_column = data->config->columns; config_column != NULL; config_column = config_column->next)
		{
			if (!config_column->hidden && column_index != unique_id_column_index)
				fprintf(data->config->output_stream, ", %s", config_column->info->name);

			column_index++;
		}

		fprintf(data->config->output_stream, ")\nVALUES('%s'", unique_id_column->string_value);

		column_index = 0;

		for (const mkm_config_column* config_column = data->config->columns; config_column != NULL; config_column = config_column->next)
		{
			if (!config_column->hidden && column_index != unique_id_column_index)
			{
				fprintf(data->config->output_stream, ", ");

				const mkm_data_column* column = &row->columns[column_index];

				switch (column->type)
				{
				case MKM_DATA_COLUMN_TYPE_STRING:	fprintf(data->config->output_stream, "'%s'", column->string_value); break;
				case MKM_DATA_COLUMN_TYPE_UINT32:	fprintf(data->config->output_stream, "%u", column->uint32_value); break;
				case MKM_DATA_COLUMN_TYPE_PRICE:	fprintf(data->config->output_stream, "%d", column->price_value); break;
				case MKM_DATA_COLUMN_TYPE_BOOL:		fprintf(data->config->output_stream, "%u", column->bool_value ? 1 : 0); break;
				default:							assert(0);
				}
			}

			column_index++;
		}

		fprintf(data->config->output_stream, ")\nON CONFLICT (unique_id) DO UPDATE SET\n");

		column_index = 0;

		for (const mkm_config_column* config_column = data->config->columns; config_column != NULL; config_column = config_column->next)
		{
			if (!config_column->hidden && column_index != unique_id_column_index)
			{
				fprintf(data->config->output_stream, "%s = EXCLUDED.%s", config_column->info->name, config_column->info->name);

				if(!mkm_output_sql_is_last_visible_column(config_column))
					fprintf(data->config->output_stream, ",");
				else
					fprintf(data->config->output_stream, ";");

				fprintf(data->config->output_stream, "\n");
			}

			column_index++;
		}
	}
}
