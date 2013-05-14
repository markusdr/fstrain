project (glue)

include_directories("${CMAKE_SOURCE_DIR}/..")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${OPENFST_INCLUDE_DIR})

add_library(${PROJECT_NAME}
  ${PROJECT_SOURCE_DIR}/create+train-r-interface.cc
  ${PROJECT_SOURCE_DIR}/create+train-r-interface2.cc
  ${PROJECT_SOURCE_DIR}/create+train-r-interface3.cc
)

set(LINK_DEPENDENCIES ${OPENFST_LIB} util core create train)

target_link_libraries(${PROJECT_NAME} ${LINK_DEPENDENCIES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "libfstrain-")

set(feature_libs
  create+train-r-interface
  create+train-r-interface2
  create+train-r-interface3
)
foreach(name ${feature_libs})
  message(${name})
  add_library(${name} SHARED ${PROJECT_SOURCE_DIR}/${name}.cc)
  target_link_libraries(${name} ${LINK_DEPENDENCIES})
  set_target_properties(${name} PROPERTIES PREFIX "")
endforeach()
