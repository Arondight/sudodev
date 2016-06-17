cmake_minimum_required (VERSION 2.6 FATAL_ERROR)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set (INIT_SCRIPT_PATH ${PROJECT_SOURCE_DIR}/init)
set (SOURCE_PATH ${PROJECT_SOURCE_DIR}/src)

if(CMAKE_COMPILER_IS_GNUCXX)
  set (CMAKE_C_FLAGS "-Wall -lpthread")
  set (CMAKE_C_FLAGS_DEBUG "-g3")
  set (CMAKE_C_FLAGS_RELEASE "-O3")
  set (CMAKE_EXE_LINKER_FLAGS "-s")
endif()

if(NOT UNIX)
  message (FATAL_ERROR "Only Unix-like system is supported, quit.")
endif ()

include_directories (${SOURCE_PATH})

add_subdirectory (${SOURCE_PATH})
add_subdirectory (${INIT_SCRIPT_PATH})

set (BINARYS ${EXECUTABLE_OUTPUT_PATH}/sudodev ${EXECUTABLE_OUTPUT_PATH}/sudodevd)
install (PROGRAMS ${BINARYS} DESTINATION ${CMAKE_INSTALL_PREFIX}/usr/bin)

