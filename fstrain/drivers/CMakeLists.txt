project (drivers)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

set(LINK_DEPENDENCIES
  ${OPENFST_LIB}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  core util create train)

add_executable(transducer-data-loglik ${PROJECT_SOURCE_DIR}/transducer-data-loglik.cc)
target_link_libraries(transducer-data-loglik ${LINK_DEPENDENCIES})

add_executable(transducer-decode ${PROJECT_SOURCE_DIR}/transducer-decode.cc)
target_link_libraries(transducer-decode ${LINK_DEPENDENCIES})

add_executable(transducer-train ${PROJECT_SOURCE_DIR}/transducer-train.cc)
target_link_libraries(transducer-train ${LINK_DEPENDENCIES})

