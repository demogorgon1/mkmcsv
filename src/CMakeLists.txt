cmake_minimum_required (VERSION 3.19)

add_executable(mkmcsv
	main.c
	mkm_base.h
	mkm_config.c
	mkm_config.h
	mkm_csv.c
	mkm_csv.h
	mkm_error.c
	mkm_error.h
	mkm_output.c
	mkm_output.h
	mkm_output_text.c
	mkm_output_text.h
	mkm_price.c
	mkm_price.h)
	
target_link_libraries(mkmcsv sfc::sfc)

#add_test(mkmcsv <command>)