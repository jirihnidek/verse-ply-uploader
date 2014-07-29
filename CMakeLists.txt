# Minimal version of CMake
cmake_minimum_required (VERSION 2.6)

# Build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif ()

# Define project name
project (verse-ply-uploader)

# The version number
set (${PROJECT_NAME}_VERSION_MAJOR 0)
set (${PROJECT_NAME}_VERSION_MINOR 1)
set (${PROJECT_NAME}_PATCH_LEVEL 0)

set (${PROJECT_NAME}_VERSION
	"${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}")

option(LIBRPLY_CLANG "Use Clang Compiler" OFF)

# Clang compiler
if (LIBRPLY_CLANG)
	set (CMAKE_C_COMPILER "/usr/bin/clang")
	set (CMAKE_CC_COMPILER "/usr/bin/clang++")
endif (LIBRPLY_CLANG)

# Set up directory with 3rd party cmake modules
set (CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build_files/cmake/modules/")

# Set output directory for binaries
set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/)

# Try to find Verse library (required)
find_package (Verse REQUIRED)

# Try to find RPLY package (required)
find_package (RPLY REQUIRED)

# Try to find OpenGL
find_package (OpenGL)

# Try to find GLUT
find_package (GLUT)

# Set source code of Verse PLY uploader
set (verse_ply_uploader_src ./src/main.c)

# Include directories
include_directories (./src)
include_directories (${VERSE_INCLUDE_DIR})
include_directories (${RPLY_INCLUDE_DIR})
include_directories (${OPENGL_INCLUDE_DIR})
include_directories (${GLUT_INCLUDE_DIR})

set ( verse_ply_uploader_libs ${VERSE_LIBRARIES} ${RPLY_LIBRARIES} )

# Make build flags compiler specific for verse_server
if (CMAKE_COMPILER_IS_GNUCC)
    set (CMAKE_C_FLAGS "-D_REETRANT -Wall -Wextra -pedantic -Wno-long-long")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
   	    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -O0")
	elseif( CMAKE_BUILD_TYPE STREQUAL "Release" )
	    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNDEBUG -O3 -fno-strict-aliasing")
	endif ()
endif (CMAKE_COMPILER_IS_GNUCC)

# Set up dump executables
add_executable (verse_ply_uploader ${verse_ply_uploader_src})
target_link_libraries (verse_ply_uploader ${verse_ply_uploader_libs} )
