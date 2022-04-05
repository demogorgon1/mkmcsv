cmake_minimum_required (VERSION 3.19)

add_executable(mkmcsv
	main.c
	mkm_base.h
	mkm_config.c
	mkm_config.h
	mkm_csv.c
	mkm_csv.h
	mkm_data.c
	mkm_data.h
	mkm_error.c
	mkm_error.h
	mkm_input_csv.c
	mkm_input_csv.h
	mkm_output_text.c
	mkm_output_text.h
	mkm_price.c
	mkm_price.h
	test.c
	test.h)
	
target_link_libraries(mkmcsv sfc::sfc)

if(MKM_TEST)
	add_test(mkmcsv-test mkmcsv test)
endif()