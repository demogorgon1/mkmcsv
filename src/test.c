#ifdef MKM_TEST

#include <stdio.h>
#include <stdlib.h>

#include <sfc/sfc.h>

#include "mkm_config.h"
#include "mkm_data.h"

#define TEST_ASSERT(_expression) test_assert(_expression, #_expression, __FILE__, __LINE__)

void
test_assert(
	int			result,
	const char* message,
	const char* file,
	int			line)
{
	if (!result)
	{
		printf("FAILED: %s (%s:%d)\n", message, file, line);

		exit(1);
	}
}

void
test_write_file(
	const char*	path,
	const char*	text)
{
	FILE* f = fopen(path, "wb");
	TEST_ASSERT(f != NULL);

	size_t len = strlen(text);
	size_t written = fwrite(text, 1, len, f);
	TEST_ASSERT(written == len);

	fclose(f);
}

void 
test_compare_file(
	const char* path,
	const char* text)
{
	FILE* f = fopen(path, "rb");
	TEST_ASSERT(f != NULL);

	size_t text_len = strlen(text);
	
	fseek(f, 0, SEEK_END);
	size_t file_len = ftell(f);
	fseek(f, 0, SEEK_SET);

	TEST_ASSERT(file_len == text_len);
	
	char* p = (char*)malloc(file_len + 1);
	TEST_ASSERT(p != NULL);

	size_t bytes_read = fread(p, 1, file_len, f);
	TEST_ASSERT(bytes_read == file_len);
	p[file_len] = '\0';

	fclose(f);

	TEST_ASSERT(strcmp(p, text) == 0);
}

typedef struct _test_http_context
{
	sfc_app*			app;
	size_t				request_count;
} test_http_context;

typedef struct _test_http_request
{
	test_http_context*	context;
	char*				result;
	size_t				size;
} test_http_request;

void*
test_http_context_create(
	struct _sfc_app*	app)
{
	test_http_context* context = (test_http_context*)SFC_ALLOC(app->alloc, app->user_data, NULL, sizeof(test_http_context));
	TEST_ASSERT(context != NULL);
	memset(context, 0, sizeof(test_http_context));

	context->app = app;

	return context;
}

void
test_http_context_destroy(
	void*				http_context)
{
	test_http_context* context = (test_http_context*)http_context;

	/* Make sure there are no pending requests */
	TEST_ASSERT(context->request_count == 0);

	context->app->free(context->app->user_data, http_context);
}

sfc_result
test_http_context_update(
	void*				http_context)
{
	MKM_UNUSED(http_context);

	return SFC_RESULT_OK;
}

char*
test_strdup_no_null_term(
	sfc_app*			app,
	const char*			string,
	size_t*				out_len)
{
	*out_len = strlen(string);
	char* new_string = (char*)SFC_ALLOC(app->alloc, app->user_data, NULL, *out_len);
	memcpy(new_string, string, *out_len);
	return new_string;
}

void*
test_http_get(
	void*				http_context,
	const char*			url)
{
	test_http_context* context = (test_http_context*)http_context;

	test_http_request* req = (test_http_request*)SFC_ALLOC(context->app->alloc, context->app->user_data, NULL, sizeof(test_http_request));
	assert(req != NULL);
	memset(req, 0, sizeof(test_http_request));

	req->context = context;

	context->request_count++;
	
	if (strcmp(url, "https://api.scryfall.com/cards/cardmarket/100") == 0)
	{
		req->result = test_strdup_no_null_term(context->app,
			"{\"object\":\"card\", \"name\":\"test100\", \"set\":\"test\", \"collector_number\":\"1\"}",
			&req->size);
	}
	else if (strcmp(url, "https://api.scryfall.com/cards/cardmarket/101") == 0)
	{
		req->result = test_strdup_no_null_term(context->app,
			"{\"object\":\"card\", \"name\":\"test101\", \"set\":\"test\", \"collector_number\":\"2\"}",
			&req->size);
	}
	else
	{
		TEST_ASSERT(0);
	}

	return req;
}

sfc_bool
test_http_poll(
	void*				http_request,
	sfc_result*			out_result,
	char**				out_data,
	size_t*				out_data_size)
{
	test_http_request* req = (test_http_request*)http_request;
	TEST_ASSERT(req->result != NULL);
	TEST_ASSERT(req->context->request_count > 0);

	*out_data = req->result;
	*out_data_size = req->size;
	*out_result = SFC_RESULT_OK;

	req->context->request_count--;

	req->context->app->free(req->context->app->user_data, req);

	return SFC_TRUE;
}

void
test_csv_in_text_out()
{
	test_write_file("tmp_mkm_test_0.txt", 
		"idProduct;groupCount;price;idLanguage;condition;isFoil;isSigned;isAltered;isPlayset;isReverseHolo;isFirstEd;isFullArt;isUberRare;isWithDie\n"
		"100;1;10;1;1;;;;;;;;;\n"
		"101;1;11;1;1;;;;;;;;;\n");

	char* argv[] =
	{
		"",
		"--columns", "name+price+condition",
		"--input", "csv",
		"--output", "text",
		"tmp_mkm_test_0.txt"
	};

	mkm_config config;
	mkm_config_init(&config, sizeof(argv) / sizeof(char*), argv);

	config.output_stream = fopen("tmp_mkm_test_1.txt", "wb");
	TEST_ASSERT(config.output_stream != NULL);

	sfc_app app;
	sfc_app_init_defaults(&app);

	app.http_create_context = test_http_context_create;
	app.http_destroy_context = test_http_context_destroy;
	app.http_update_context = test_http_context_update;
	app.http_get = test_http_get;
	app.http_poll = test_http_poll;

	sfc_cache* cache = sfc_cache_create(&app, 9);
	TEST_ASSERT(cache != NULL);

	mkm_data* data = mkm_data_create(&config, cache);

	config.input_callback(data);
	config.output_callback(data);

	mkm_data_destroy(data);
	sfc_cache_destroy(cache);
	mkm_config_uninit(&config);

	fclose(config.output_stream);

	test_compare_file("tmp_mkm_test_1.txt", 
		"name   |price|condition|\n"
		"-------|-----|---------|\n"
		"test100|10.00|        1|\n"
		"test101|11.00|        1|\n");
}

int	
test(
	int		argc,
	char**	argv)
{
	MKM_UNUSED(argc);
	MKM_UNUSED(argv);

	test_csv_in_text_out();

	return 0;
}

#endif /* MKM_TEST */