cmake_minimum_required( VERSION 3.12 )
project( ai-threadsafe-testsuite
         LANGUAGES CXX
         DESCRIPTION "Testsuite for git submodule ai-threadsafe." )

# This project uses aicxx modules.
include( cwm4/cmake/AICxxProject )

#Exit if someone tries to contaminates the source directory with an in-source build.
if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
  message( FATAL_ERROR "Please do out-of-source builds.\nCleanup: \"rm -rf CMake CMakeCache.txt CMakeFiles/\"" )
endif ()

add_subdirectory( cwds )
add_subdirectory( utils )
add_subdirectory( threadsafe )
add_subdirectory( microbench )
add_subdirectory( src )
