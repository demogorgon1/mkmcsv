#include <stdio.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_csv.h"
#include "mkm_output.h"
#include "mkm_output_text.h"

int
main(
	int			argc, 
	char**		argv)
{
	mkm_config config;
	mkm_config_init(&config, argc, argv);

	sfc_app app;
	sfc_app_init_defaults(&app);

	sfc_cache* cache = sfc_cache_create(&app, 9);
	assert(cache != NULL);

	{
		sfc_result result = sfc_cache_load(cache, config.cache_file);
		if(result != SFC_RESULT_OK && result != SFC_RESULT_FILE_OPEN_ERROR)
			mkm_error("Failed to load cache: %s (%d)", config.cache_file, result);
	}

	mkm_output* output = mkm_output_create(&config, cache);

	switch(config.input_type)
	{
	case MKM_CONFIG_INPUT_TYPE_CSV:
		for(const mkm_config_input_file* input_file = config.input_files; input_file != NULL; input_file = input_file->next)
		{
			mkm_csv* csv = mkm_csv_create_from_file(input_file->path);
			mkm_output_process_csv(output, csv);
			mkm_csv_destroy(csv);
		}
		break;
	default:
		assert(0);
	}

	switch(config.output_type)
	{
	case MKM_CONFIG_OUTPUT_TYPE_TEXT:
		mkm_output_text(output);
		break;
	default:
		assert(0);
	}

	mkm_output_destroy(output);

	{
		sfc_result result = sfc_cache_save(cache, config.cache_file);
		if(result != SFC_RESULT_OK)
			mkm_error("Failed to save cache: %s (%d)", config.cache_file, result);
	}

	sfc_cache_destroy(cache);

	mkm_config_uninit(&config);

	return 0;
}