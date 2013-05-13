project (core)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/expectations.cc
)

set(LINK_DEPENDENCIES ${OPENFST_LIB} dl)

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")

add_library(expectation-arc SHARED ${PROJECT_SOURCE_DIR}/expectation-arc-inline.cc)
target_link_libraries(expectation-arc ${LINK_DEPENDENCIES})
set_target_properties(expectation-arc PROPERTIES PREFIX "")

add_executable(test-construct ${PROJECT_SOURCE_DIR}/test/test-construct.cc)
target_link_libraries(test-construct ${LINK_DEPENDENCIES} core)

add_executable(test-neglog-of-signed-num ${PROJECT_SOURCE_DIR}/test/test-neglog-of-signed-num.cc)
target_link_libraries(test-neglog-of-signed-num ${LINK_DEPENDENCIES} core)
