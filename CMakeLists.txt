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

# enable_testing()

include(FetchContent)
include(GNUInstallDirs)

FetchContent_Declare(sfc
  GIT_REPOSITORY https://github.com/demogorgon1/scryfallcache.git
  GIT_TAG        283a99f3c00ef52d558260c55bace7905e71c04a # 0.1.3
)

FetchContent_MakeAvailable(sfc)

# --------------------------------------------------------------------

add_subdirectory(src)
