project (create)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/create-ngram-fst.cc
  ${PROJECT_SOURCE_DIR}/create-ngram-fst-from-best-align-r2191.cc
  ${PROJECT_SOURCE_DIR}/get-features.cc
  ${PROJECT_SOURCE_DIR}/get-well-formed.cc
  ${PROJECT_SOURCE_DIR}/v3-get-first-and-last.cc
  ${PROJECT_SOURCE_DIR}/create-ngram-fst-from-best-align.cc
  ${PROJECT_SOURCE_DIR}/create-scoring-fst-from-trie.cc
  ${PROJECT_SOURCE_DIR}/get-first-and-last.cc
  ${PROJECT_SOURCE_DIR}/ngram-fsa-insert-features.cc
  ${PROJECT_SOURCE_DIR}/wfutil.cc
  )

set(LINK_DEPENDENCIES ${OPENFST_LIB})

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")
