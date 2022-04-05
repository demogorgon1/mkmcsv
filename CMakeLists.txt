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
  GIT_TAG        896571dc3e1626fdefe9e42ddd66ad1c525539d7 # 0.1.5
)

FetchContent_MakeAvailable(sfc)

# --------------------------------------------------------------------

add_subdirectory(src)
