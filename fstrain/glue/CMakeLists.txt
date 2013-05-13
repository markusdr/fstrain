project (glue)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/create+train-r-interface2.cc
  ${PROJECT_SOURCE_DIR}/create+train-r-interface3.cc
  ${PROJECT_SOURCE_DIR}/create+train-r-interface.cc
)

set(LINK_DEPENDENCIES ${OPENFST_LIB})

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")
