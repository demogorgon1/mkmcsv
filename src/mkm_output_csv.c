#include <stdio.h>

#include "mkm_base.h"
#include "mkm_config.h"
#include "mkm_data.h"
#include "mkm_output_csv.h"

void	
mkm_output_csv(
	struct _mkm_data*			data)
{
	/* Output header */
	{
		mkm_bool first_column = MKM_TRUE;

		for (const mkm_config_column* config_column = data->config->columns; config_column != NULL; config_column = config_column->next)
		{
			if(!config_column->hidden)
			{
				if(first_column)
					first_column = MKM_FALSE;
				else
					fprintf(data->config->output_stream, ";");
			
				fprintf(data->config->output_stream, "%s", config_column->info->name);
			}
		}

		fprintf(data->config->output_stream, "\n");
	}

	/* Output rows */
	for (const mkm_data_row* row = data->first_row; row != NULL; row = row->next)
	{
		if (row->removed)
			continue;

		size_t column_index = 0;

		for (const mkm_config_column* config_column = data->config->columns; config_column != NULL; config_column = config_column->next)
		{
			if (!config_column->hidden)
			{
				if (column_index > 0)
					fprintf(data->config->output_stream, ";");

				const mkm_data_column* column = &row->columns[column_index];

				switch(column->type)
				{
				case MKM_DATA_COLUMN_TYPE_STRING:	fprintf(data->config->output_stream, "%s", column->string_value); break;
				case MKM_DATA_COLUMN_TYPE_UINT32:	fprintf(data->config->output_stream, "%u", column->uint32_value); break;
				case MKM_DATA_COLUMN_TYPE_PRICE:	fprintf(data->config->output_stream, "%d", column->price_value); break;
				case MKM_DATA_COLUMN_TYPE_BOOL:		fprintf(data->config->output_stream, "%u", column->bool_value ? 1 : 0); break;
				default:							assert(0);
				}
			}

			column_index++;
		}

		fprintf(data->config->output_stream, "\n");
	}
}
