project (util)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/approx-determinize.cc
  ${PROJECT_SOURCE_DIR}/data.cc
  ${PROJECT_SOURCE_DIR}/get-highest-feature-index.cc
  ${PROJECT_SOURCE_DIR}/load-library.cc
  ${PROJECT_SOURCE_DIR}/memory-info.cc
  ${PROJECT_SOURCE_DIR}/misc.cc
  ${PROJECT_SOURCE_DIR}/options.cc
  ${PROJECT_SOURCE_DIR}/print-path.cc
  ${PROJECT_SOURCE_DIR}/string-to-fst.cc
  ${PROJECT_SOURCE_DIR}/timer.cc
)

set(LINK_DEPENDENCIES ${OPENFST_LIB})

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")
