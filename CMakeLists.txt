cmake_minimum_required (VERSION 3.19)

project(mkmcsv)

macro(mkmcsv_option OPTION_NAME OPTION_TEXT OPTION_DEFAULT)
    option(${OPTION_NAME} ${OPTION_TEXT} ${OPTION_DEFAULT})
    if(DEFINED ENV{${OPTION_NAME}})
        # Allow overriding the option through an environment variable
        set(${OPTION_NAME} $ENV{${OPTION_NAME}})
    endif()
    if(${OPTION_NAME})
        add_definitions(-D${OPTION_NAME})
    endif()
    message(STATUS "${OPTION_NAME}: ${${OPTION_NAME}}")
endmacro()

mkmcsv_option(MKM_TEST "Enable test code (run with 'mkmcsv test')." OFF)

enable_testing()

include(FetchContent)
include(GNUInstallDirs)

FetchContent_Declare(sfc
  GIT_REPOSITORY https://github.com/demogorgon1/scryfallcache.git
  GIT_TAG        e22c622d30c6466594621f348e2d482fc1d0f497 # 0.1.10
)

FetchContent_MakeAvailable(sfc)

# --------------------------------------------------------------------

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-function)
endif()

add_subdirectory(src)
