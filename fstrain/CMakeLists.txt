cmake_minimum_required (VERSION 2.6)
project (fstrain)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
message (${CMAKE_MODULE_PATH})

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release
    CACHE STRING "Choose the type of build: None Debug Release."
    FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
message("# Current build type: ${CMAKE_BUILD_TYPE}")

## Find Boost
# set(Boost_DEBUG 1)
set(BOOST_ROOT $ENV{BOOST_ROOT})
if (NOT DEFINED BOOST_ROOT)
  message(FATAL_ERROR "No Boost found. Please specify BOOST_ROOT.")
endif()
# set(Boost_NO_SYSTEM_PATHS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost ${Boost_find_version} REQUIRED COMPONENTS
  program_options
  filesystem
  system
  )

find_package(OpenFst)

add_subdirectory(core)
add_subdirectory(util)
add_subdirectory(create)
add_subdirectory(train)
add_subdirectory(glue)
add_subdirectory(drivers)

