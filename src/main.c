#include <stdio.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_error.h"
#include "mkm_data.h"
#include "mkm_output_sql.h"

#include "test.h"

int
main(
	int			argc, 
	char**		argv)
{
	#if defined(MKM_TEST)
		if(argc > 1 && strcmp(argv[1], "test") == 0)
			return test(argc, argv);
	#endif

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

	mkm_data* data = mkm_data_create(&config, cache);

	config.input_callback(data);

	mkm_data_sort(data);

	config.output_callback(data);

	mkm_data_destroy(data);

	{
		sfc_result result = sfc_cache_save(cache, config.cache_file);
		if(result != SFC_RESULT_OK)
			mkm_error("Failed to save cache: %s (%d)", config.cache_file, result);
	}

	sfc_cache_destroy(cache);

	mkm_config_uninit(&config);

	return 0;
}