cmake_minimum_required (VERSION 2.6)

set (CMAKE_CXX_COMPILER "gcc")
set (CMAKE_CXX_FLAGS "-Wall -lpthread")
set (CMAKE_CXX_FLAGS_DEBUG "-g3")
set (CMAKE_CXX_FLAGS_RELEASE "-O3")
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories (${PROJECT_SOURCE_DIR}/src)
add_subdirectory (${PROJECT_SOURCE_DIR}/src)

set (
  BINARYS
  ${PROJECT_SOURCE_DIR}/bin/sudodev
  ${PROJECT_SOURCE_DIR}/bin/sudodevd
  )
install (PROGRAMS ${BINARYS} DESTINATION /usr/bin)

