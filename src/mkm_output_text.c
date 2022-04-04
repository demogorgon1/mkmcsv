#include <stdio.h>
#include <string.h>

#include "mkm_base.h"
#include "mkm_config.h"
#include "mkm_output.h"
#include "mkm_output_text.h"

void
mkm_output_text_render_cell(
	const mkm_output_column*	column,
	char*						buffer,
	size_t						buffer_size)
{
	switch(column->type)
	{
	case MKM_OUTPUT_COLUMN_TYPE_BOOL:		snprintf(buffer, buffer_size, "%s", column->bool_value ? "true" : "false"); break;
	case MKM_OUTPUT_COLUMN_TYPE_INTEGER:	snprintf(buffer, buffer_size, "%u", column->integer_value); break;
	case MKM_OUTPUT_COLUMN_TYPE_STRING:		snprintf(buffer, buffer_size, "%s", column->string_value); break;
	default:
		assert(0);
	}
}

void
mkm_output_text_repeating_chars(
	char						character,
	size_t						count)
{
	for(size_t i = 0; i < count; i++)
		fprintf(stdout, "%c", character);
}

/*---------------------------------------------------------------------*/

void	
mkm_output_text(
	struct _mkm_output*	output)
{
	/* We have to find the longest table cell width per column */
	struct column_info
	{
		size_t							width;
		const char*						header;
	};

	struct column_info* columns = (struct column_info*)malloc(output->config->num_output_columns * sizeof(struct column_info));
	
	size_t column_index = 0;

	for (const mkm_config_output_column* column = output->config->output_columns; column != NULL; column = column->next)
	{
		assert(column_index < output->config->num_output_columns);

		size_t cell_width = strlen(column->info->name);

		for(const mkm_output_row* row = output->first_row; row != NULL; row = row->next)
		{
			char buffer[256];
			mkm_output_text_render_cell(&row->columns[column_index], buffer, sizeof(buffer));
			size_t len = strlen(buffer);
			if(len > cell_width)
				cell_width = len;
		}

		columns[column_index].width = cell_width;
		columns[column_index].header = column->info->name;

		column_index++;
	}

	/* Output header */
	{
		struct column_info* column = columns;
		for (size_t i = 0; i < output->config->num_output_columns; i++)
		{
			fprintf(stdout, "%s", column->header);

			size_t header_length = strlen(column->header);
			assert(header_length <= column->width);
			mkm_output_text_repeating_chars(' ', column->width - header_length);

			fprintf(stdout, "|");

			column++;
		}

		fprintf(stdout, "\n");
	}

	/* Output header line */
	{
		struct column_info* column = columns;
		for (size_t i = 0; i < output->config->num_output_columns; i++)
		{
			mkm_output_text_repeating_chars('-', column->width);
			fprintf(stdout, "|");
			column++;
		}

		fprintf(stdout, "\n");
	}

	/* Output rows */
	for (const mkm_output_row* row = output->first_row; row != NULL; row = row->next)
	{
		struct column_info* column = columns;
		for (size_t i = 0; i < output->config->num_output_columns; i++)
		{
			char buffer[256];
			mkm_output_text_render_cell(&row->columns[i], buffer, sizeof(buffer));
			size_t len = strlen(buffer);

			assert(len <= column->width);
			mkm_output_text_repeating_chars(' ', column->width - len);

			fprintf(stdout, "%s|", buffer);

			column++;
		}

		fprintf(stdout, "\n");
	}

	free(columns);
}
