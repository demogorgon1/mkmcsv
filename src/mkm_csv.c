#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mkm_csv.h"
#include "mkm_error.h"
#include "mkm_price.h"

#define MAX_CSV_LINE_LENGTH	512
#define MAX_CSV_COLUMNS		64

typedef struct _mkm_csv_line
{
	size_t			num_columns;
	const char*		columns[MAX_CSV_COLUMNS];
	uint8_t			column_enums[MAX_CSV_COLUMNS];
	char			string[MAX_CSV_LINE_LENGTH];
} mkm_csv_line;

mkm_bool
mkm_csv_read_line(
	FILE*				f,
	mkm_csv_line*		line)
{
	memset(line, 0, sizeof(mkm_csv_line));

	char* p = fgets(line->string, sizeof(mkm_csv_line) - 1, f);
	if(p == NULL)
		return MKM_FALSE;

	for(;;)
	{
		/* Find next delimiter or EOL */
		size_t token_length = 0;
		while(p[token_length] != ';' && p[token_length] != '\0')
			token_length++;

		mkm_bool end_of_line = p[token_length] == '\0';

		p[token_length] = '\0';
		
		/* Remove any new-lines */
		for(size_t i = 0; i < token_length; i++)
		{
			if(p[i] == '\n' || p[i] == '\r')
				p[i] = '\0';
		}

		/* Add column */
		MKM_ERROR_CHECK(line->num_columns < MAX_CSV_COLUMNS, "Too many columns in CSV file.");
		line->columns[line->num_columns++] = p;

		if(end_of_line)
			break;

		p += token_length + 1;
	}

	return MKM_TRUE;
}

uint32_t
mkm_csv_column_to_uint32(
	const char*			column,
	uint8_t				column_enum)
{
	switch(column_enum)
	{
	case MKM_CSV_COLUMN_PRICE:
		return mkm_price_parse(column);

	default:
		return (uint32_t)strtoul(column, NULL, 10);
	}
}

/*------------------------------------------------------------------*/

mkm_csv* 
mkm_csv_create_from_file(
	const char*			path)
{
	FILE* f = fopen(path, "r");
	MKM_ERROR_CHECK(f != NULL, "Failed to open file: %s", path);

	mkm_csv_line header_line;

	/* Initialize header line */
	{
		MKM_ERROR_CHECK(mkm_csv_read_line(f, &header_line), "Failed to read CSV header line from: %s", path);

		for (size_t i = 0; i < header_line.num_columns; i++)
		{
			const char* p = header_line.columns[i];
			uint8_t e = MKM_CSV_COLUMN_UNUSED;
		
			if(strcmp(p, "idProduct") == 0)
				e = MKM_CSV_COLUMN_ID_PRODUCT;
			else if (strcmp(p, "price") == 0)
				e = MKM_CSV_COLUMN_PRICE;
			else if (strcmp(p, "idLanguage") == 0)
				e = MKM_CSV_COLUMN_ID_LANGUAGE;
			else if (strcmp(p, "condition") == 0)
				e = MKM_CSV_COLUMN_CONDITION;
			else if (strcmp(p, "isFoil") == 0)
				e = MKM_CSV_COLUMN_IS_FOIL;
			else if (strcmp(p, "isSigned") == 0)
				e = MKM_CSV_COLUMN_IS_SIGNED;
			else if (strcmp(p, "isAltered") == 0)
				e = MKM_CSV_COLUMN_IS_ALTERED;
			else if (strcmp(p, "isPlayset") == 0)
				e = MKM_CSV_COLUMN_IS_PLAYSET;
		
			header_line.column_enums[i] = e;
		}
	}

	/* Read CSV rows */
	mkm_csv* csv = MKM_NEW(mkm_csv);

	uint32_t line_num = 1;
	mkm_csv_row* last_row = NULL;

	mkm_csv_line line;
	while(mkm_csv_read_line(f, &line))
	{
		MKM_ERROR_CHECK(header_line.num_columns == line.num_columns, "CSV column count mismatch on line %u: %s", line_num, path);
	
		/* Allocate row and insert in linked list */
		mkm_csv_row* row = MKM_NEW(mkm_csv_row);
		if(last_row != NULL)
			last_row->next = row;
		else
			csv->rows = row;

		last_row = row;

		/* Convert columns */
		for(size_t i = 0; i < line.num_columns; i++)
		{
			uint8_t e = header_line.column_enums[i];
			assert(e < NUM_MKM_CSV_COLUMNS);

			if(e != MKM_CSV_COLUMN_UNUSED)
			{
				row->flags |= 1 << e;
				row->columns[e] = mkm_csv_column_to_uint32(line.columns[i], e);
			}
		}

		line_num++;
	}

	fclose(f);

	return csv;
}

void		
mkm_csv_destroy(
	mkm_csv*			csv)
{
	/* Free rows */
	{
		mkm_csv_row* row = csv->rows;
		while(row != NULL)
		{
			mkm_csv_row* next = row->next;
			free(row);
			row = next;
		}
	}

	free(csv);
}

mkm_bool	
mkm_csv_row_has_column(
	const mkm_csv_row*	row,
	uint8_t				index)
{
	return row->flags & (1 << index);
}
