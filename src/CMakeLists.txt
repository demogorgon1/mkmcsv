cmake_minimum_required (VERSION 3.19)

file(GLOB C_FILES "*.c")
file(GLOB H_FILES "*.h")

if(NOT MKM_TEST)
	list(FILTER C_FILES EXCLUDE REGEX ".*test\\.c$")
	list(FILTER H_FILES EXCLUDE REGEX ".*test\\.h$")
endif()

add_executable(mkmcsv ${C_FILES} ${H_FILES})
	
target_link_libraries(mkmcsv sfc::sfc)

install(TARGETS mkmcsv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(MKM_TEST)
	add_test(mkmcsv-test mkmcsv test)
endif()