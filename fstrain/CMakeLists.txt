cmake_minimum_required (VERSION 2.6)
project (fstrain)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
message (STATUS "Using cmake package path: ${CMAKE_MODULE_PATH}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC") 

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release
    CACHE STRING "Choose the type of build: None Debug Release."
    FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
message(STATUS "Current build type: ${CMAKE_BUILD_TYPE}")

## Find Boost
# set(Boost_DEBUG 1) 
if(DEFINED BOOST_ROOT)
  message("Will look for Boost in ${BOOST_ROOT}")
  set(Boost_NO_SYSTEM_PATHS ON)
  set(Boost_FIND_OPTIONS NO_CMAKE_SYSTEM_PATH)
endif()
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost ${Boost_find_version} REQUIRED COMPONENTS
  program_options
  filesystem
  unit_test_framework
  system
  thread
  )
if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found")
else()
  message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
endif()

find_package(OpenFst)

foreach(dir
    core
    util
    create
    train
    glue
    drivers
    )
  add_subdirectory(${dir})
endforeach()
