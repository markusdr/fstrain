project (train)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/lenmatch.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-conditional.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-conditional-lenmatch.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-conditional-lenmatch-r-interface.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-condother-lenmatch.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-condother-lenmatch-r-interface.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-factory.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-joint.cc
  ${PROJECT_SOURCE_DIR}/obj-func-fst-r-interface.cc
  ${PROJECT_SOURCE_DIR}/set-feature-weights.cc
)

set(LINK_DEPENDENCIES ${OPENFST_LIB})

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")
