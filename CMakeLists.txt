cmake_minimum_required (VERSION 2.6)

set (CMAKE_C_COMPILER "gcc")
set (CMAKE_C_FLAGS "-Wall -lpthread")
set (CMAKE_C_FLAGS_DEBUG "-g3")
set (CMAKE_C_FLAGS_RELEASE "-O3")
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (INIT_SCRIPT_PATH ${PROJECT_SOURCE_DIR}/init)
set (SOURCE_PATH ${PROJECT_SOURCE_DIR}/src)

include_directories (${SOURCE_PATH})

add_subdirectory (${SOURCE_PATH})
add_subdirectory (${INIT_SCRIPT_PATH})

set (
  BINARYS
  ${PROJECT_SOURCE_DIR}/bin/sudodev
  ${PROJECT_SOURCE_DIR}/bin/sudodevd
)
install (PROGRAMS ${BINARYS} DESTINATION /usr/bin)

