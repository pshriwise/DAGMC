project(DAGMC)
cmake_minimum_required(VERSION 3.1)
enable_language(CXX)

# Set DAGMC version
set(DAGMC_MAJOR_VERSION 3)
set(DAGMC_MINOR_VERSION 2)
set(DAGMC_PATCH_VERSION 0)
set(DAGMC_VERSION ${DAGMC_MAJOR_VERSION}.${DAGMC_MINOR_VERSION}.${DAGMC_PATCH_VERSION})

# Set DAGMC Git SHA

# Set git SHA1 hash as a compile definition
find_package(Git)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE DAGMC_GIT_SHA_SUCCESS
    OUTPUT_VARIABLE DAGMC_GIT_SHA
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT DAGMC_GIT_SHA_SUCCESS EQUAL "0")
    message(WARNING "Could not determine the commit SHA for DAGMC.")
    set(DAGMC_GIT_SHA "")
  endif()

  option(GIT_SUBMODULE "Check submodules during build" ON)
  if(GIT_SUBMODULE)
    message(STATUS "Submodule update")
    execute_process(COMMAND ${GIT_EXECUTABLE} "submodule" "update" "--init" "--recursive"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL 0)
      message(FATAL_ERROR "git submodule update --init --recursive failed with \
        ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
  endif()
endif()

# Check to see if submodules exist (by checking one)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/pyne/pyne/readme.rst")
  message(FATAL_ERROR "The git submodules were not downloaded! GIT_SUBMODULE was \
    turned off or failed. Please update submodules and try again.")
endif()

# Make the scripts in the "cmake" directory available to CMake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(DAGMC_macros)

dagmc_setup_build()

dagmc_setup_options()

if(BUILD_MCNP5 OR BUILD_MCNP6)
  enable_language(Fortran)
endif()

find_package(OpenMP)

cmake_minimum_required(VERSION 2.8)
SET(CMAKE_CXX_STANDARD 11)

# Allow use of find_package(HDF5), find_package(MOAB), etc.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

# find the gfortran library name, this is required for OS/X which uses a variety
# of package managers to install gfortan and caould be in a number of places
# if (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
#   find_library(GFORTRAN_LIBRARY NAMES gfortran libgfortran
#                                 PATHS /usr/local/lib/gcc/* /lib /lib64 /usr/lib
#                                       /usr/lib64 /usr/lib/gcc/*/*)
# endif (${CMAKE_SYSTEM_NAME} STREQUAL Darwin)

# Make sure we have HDF5
find_package(HDF5 REQUIRED)
find_package(MBVH QUIET)

# Make sure we have MOAB
find_package(MOAB REQUIRED)
set(DAG_LINK_LIBS ${MOAB_LIBRARIES})
string(STRIP "${DAG_LINK_LIBS}" DAG_LINK_LIBS)
string(REGEX REPLACE ";" " " DAG_LINK_LIBS "${DAG_LINK_LIBS}")
string(REGEX REPLACE "[ ]+" " " DAG_LINK_LIBS "${DAG_LINK_LIBS}")
separate_arguments(DAG_LINK_LIBS)
list(REMOVE_DUPLICATES DAG_LINK_LIBS)
if (BUILD_STATIC)
  list(REMOVE_ITEM DAG_LINK_LIBS z)
  list(APPEND DAG_LINK_LIBS z)
  list(APPEND DAG_LINK_LIBS gfortran)
  list(APPEND DAG_LINK_LIBS quadmath)
endif (BUILD_STATIC)
message(STATUS "DAG_LINK_LIBS: ${DAG_LINK_LIBS}")

if(MBVH_FOUND)
  add_definitions(-DSIMD_BVH)
  link_directories(${MBVH_LIBRARY_DIRS})
  include_directories(${MBVH_INCLUDE_DIRS})
  list(APPEND DAG_LINK_LIBS ${MBVH_LIBRARIES})
endif(MBVH_FOUND)


# Get some environment variables
set(ENV_USER "$ENV{USER}")
execute_process(COMMAND hostname       OUTPUT_VARIABLE ENV_HOST OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND uname -s       OUTPUT_VARIABLE ENV_OS   OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND date +%m/%d/%y OUTPUT_VARIABLE ENV_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND date +%H:%M:%S OUTPUT_VARIABLE ENV_TIME OUTPUT_STRIP_TRAILING_WHITESPACE)

# Figure out which physics codes to build
if (BUILD_ALL)
  set(BUILD_MCNP5 ON)
  set(BUILD_MCNP6 ON)
  set(BUILD_GEANT4 ON)
  set(BUILD_FLUKA ON)
endif (BUILD_ALL)
if (BUILD_MCNP5 OR BUILD_MCNP6)
  set(BUILD_TALLY ON)
endif (BUILD_MCNP5 OR BUILD_MCNP6)

# Build MCNP
if (BUILD_MCNP5 OR BUILD_MCNP6)
  set(BUILD_ANY ON)
  add_subdirectory(mcnp)
endif (BUILD_MCNP5 OR BUILD_MCNP6)

dagmc_setup_flags()

if (BUILD_TESTS)
  enable_testing()
endif ()

add_subdirectory(src)

dagmc_make_configure_files()

message("")
