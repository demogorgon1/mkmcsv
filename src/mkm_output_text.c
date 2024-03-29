#include <stdio.h>
#include <string.h>
#include <locale.h>

#include "mkm_base.h"
#include "mkm_config.h"
#include "mkm_data.h"
#include "mkm_output_text.h"

void
mkm_output_text_render_cell(
	const mkm_data_column*		column,
	char*						buffer,
	size_t						buffer_size)
{
	switch(column->type)
	{
	case MKM_DATA_COLUMN_TYPE_BOOL:		snprintf(buffer, buffer_size, "%s", column->bool_value ? "true" : "false"); break;
	case MKM_DATA_COLUMN_TYPE_UINT32:	snprintf(buffer, buffer_size, "%u", column->uint32_value); break;
	case MKM_DATA_COLUMN_TYPE_STRING:	snprintf(buffer, buffer_size, "%s", column->string_value); break;

	case MKM_DATA_COLUMN_TYPE_PRICE:	
		{
			int32_t minor = column->price_value < 0 ? (-column->price_value % 100) : (column->price_value % 100);
			int32_t major = column->price_value / 100;
			const struct lconv* l = localeconv();
			snprintf(buffer, buffer_size, "%d%s%02d", major, l->decimal_point, minor);
		}
		break;

	default:
		assert(0);
	}
}

void
mkm_output_text_repeating_chars(
	FILE*						stream,
	char						character,
	size_t						count)
{
	for(size_t i = 0; i < count; i++)
		fprintf(stream, "%c", character);
}

/*---------------------------------------------------------------------*/

void	
mkm_output_text(
	struct _mkm_data*			data)
{
	/* We have to find the longest table cell width per column */
	struct column_info
	{
		size_t							width;
		const char*						header;
		size_t							index;
	};

	struct column_info* columns = (struct column_info*)malloc(data->config->num_columns * sizeof(struct column_info));
	
	size_t column_index = 0;
	size_t visible_column_count = 0;

	for (const mkm_config_column* column = data->config->columns; column != NULL; column = column->next)
	{
		assert(column_index < data->config->num_columns);

		if(!column->hidden)
		{
			size_t cell_width = strlen(column->info->name);

			for(const mkm_data_row* row = data->first_row; row != NULL; row = row->next)
			{
				if(!mkm_data_row_should_output(data, row))
					continue;

				char buffer[256];
				mkm_output_text_render_cell(&row->columns[column_index], buffer, sizeof(buffer));
				size_t len = strlen(buffer);
				if(len > cell_width)
					cell_width = len;
			}

			columns[visible_column_count].width = cell_width;
			columns[visible_column_count].header = column->info->name;
			columns[visible_column_count].index = column_index;

			visible_column_count++;
		}

		column_index++;
	}

	if(visible_column_count == 0)
	{
		free(columns);
		return;
	}

	/* Output header */
	{
		struct column_info* column = columns;
		for (size_t i = 0; i < visible_column_count; i++)
		{
			fprintf(data->config->output_stream, "%s", column->header);

			size_t header_length = strlen(column->header);
			assert(header_length <= column->width);
			mkm_output_text_repeating_chars(data->config->output_stream, ' ', column->width - header_length);

			fprintf(data->config->output_stream, "|");

			column++;
		}

		fprintf(data->config->output_stream, "\n");
	}

	/* Output header line */
	{
		struct column_info* column = columns;
		for (size_t i = 0; i < visible_column_count; i++)
		{
			mkm_output_text_repeating_chars(data->config->output_stream, '-', column->width);
			fprintf(data->config->output_stream, "|");
			column++;
		}

		fprintf(data->config->output_stream, "\n");
	}

	/* Output rows */
	for (const mkm_data_row* row = data->first_row; row != NULL; row = row->next)
	{
		if (!mkm_data_row_should_output(data, row))
			continue;

		struct column_info* column = columns;
		for (size_t i = 0; i < visible_column_count; i++)
		{
			char buffer[256];
			mkm_output_text_render_cell(&row->columns[column->index], buffer, sizeof(buffer));
			size_t len = strlen(buffer);

			assert(len <= column->width);
			mkm_output_text_repeating_chars(data->config->output_stream, ' ', column->width - len);

			fprintf(data->config->output_stream, "%s|", buffer);

			column++;
		}

		fprintf(data->config->output_stream, "\n");
	}

	free(columns);
}
