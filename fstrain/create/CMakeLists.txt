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

set(LINK_DEPENDENCIES ${OPENFST_LIB} ${Boost_LIBRARIES} util core)

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")

set(feature_libs
  simple
  add-collapsed
  simple+len
  simple+len+insdel
  simple+tlm
  la
  la1)
foreach(name ${feature_libs})
  add_library(${name} SHARED ${PROJECT_SOURCE_DIR}/features/${name}.cc)
  target_link_libraries(${name} ${LINK_DEPENDENCIES})
  set_target_properties(${name} PROPERTIES PREFIX "")
endforeach()

###

# file(GLOB tests "${PROJECT_SOURCE_DIR}/test/*.cc")
set(tests
  test-approx-dist
  test-backoff
  test-create-1
  test-create-2
  test-create-3
  test-create-from-align2
  test-create-ngram-fst-from-best-align
  test-create-scoring-fsa-from-data
  test-custom-phi-matcher
  test-features
  test-la
  test-matrix-mult
  )

foreach(test ${tests})
  add_executable(${test} ${PROJECT_SOURCE_DIR}/test/${test}.cc)
  target_link_libraries(${test} ${LINK_DEPENDENCIES} create)
endforeach()
