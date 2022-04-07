#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_data.h"
#include "mkm_input_csv.h"

void	
mkm_input_csv(
	struct _mkm_data*		data)
{
	for (const mkm_config_input_file* input_file = data->config->input_files; input_file != NULL; input_file = input_file->next)
	{
		mkm_csv* csv = mkm_csv_create_from_file(input_file->path);
		MKM_ERROR_CHECK(csv != NULL, "Failed to load CSV: %s", input_file->path);

		mkm_data_process_csv(data, csv, NULL, NULL);
		mkm_csv_destroy(csv);
	}
}
