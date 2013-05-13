# OPENFST_FOUND
# OPENFST_DIR

find_path(OPENFST_INCLUDE_DIR fst.h ${OPENFST_ROOT}/include/fst)
find_library(OPENFST_LIB fst PATHS ${OPENFST_ROOT}/lib)

if(OPENFST_INCLUDE_DIR)
 SET(OPENFST_FOUND 1)
 STRING(REGEX REPLACE "src/include/fst$" "" OPENFST_DIR ${OPENFST_INCLUDE_DIR})
 SET(OPENFST_INCLUDE_DIR "${OPENFST_INCLUDE_DIR}/..")
 MESSAGE(STATUS "Found OpenFst: ${OPENFST_INCLUDE_DIR} and ${OPENFST_LIB}")
else()
  message(FATAL_ERROR "No OpenFst found. Please specify OPENFST_ROOT.")
ENDIF()
