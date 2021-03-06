project(project_dash)

cmake_minimum_required (VERSION 2.8)

if(POLICY CMP0054)
  cmake_policy(SET CMP0054 OLD)
endif(POLICY CMP0054)
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 OLD)
endif(POLICY CMP0053)
if(POLICY CMP0007)
  cmake_policy(SET CMP0007 OLD)
endif(POLICY CMP0007)
if(POLICY CMP0003)
  cmake_policy(SET CMP0003 OLD)
endif(POLICY CMP0003)
if(POLICY CMP0060)
  cmake_policy(SET CMP0060 OLD)
endif(POLICY CMP0060)
if(POLICY CMP0004)
  cmake_policy(SET CMP0004 OLD)
endif(POLICY CMP0004)
if(POLICY CMP0016)
  cmake_policy(SET CMP0016 OLD)
endif(POLICY CMP0016)

## Build options
option(BUILD_TESTS
       "Whether tests should be built" on)
option(BUILD_COVERAGE_TESTS
       "Whether tests are built with code coverage (lcov) support.
        Implies BUILD_TESTS. Requires GCC." off)
option(BUILD_DOCS
       "Whether doxygen documentation should be installed" off)
option(BUILD_EXAMPLES
       "Specify whether to build examples" on)
option(ENABLE_COMPILER_WARNINGS
       "Specify whether strict compiler warnings should be enabled" on)
option(WARNINGS_AS_ERRORS
       "Specify whether warnings should be treated as errors" off)
option(ENABLE_LOGGING
       "Specify whether logging should be enabled" off)
option(ENABLE_TRACE_LOGGING
       "Specify whether trace messages should be logged" off)
option(ENABLE_DART_LOGGING
       "Specify whether messages from DART should be logged" off)
option(ENABLE_ASSERTIONS
       "Specify whether runtime assertions should be checked" off)
option(ENABLE_UNIFIED_MEMORY_MODEL
       "Specify whether to assume unified memory model" on)
option(ENABLE_SHARED_WINDOWS
       "Specify whether shared memory features are enabled" on)
option(ENABLE_DEFAULT_INDEX_TYPE_LONG
       "Specify whether to use int64_t as default index type" on)
option(ENABLE_LIBNUMA
       "Specify whether libnuma features are enabled" on)
option(ENABLE_HWLOC
       "Specify whether hwloc features are enabled" on)
option(ENABLE_LIKWID
       "Specify whether likwid support is enabled" off)
option(ENABLE_PAPI
       "Specify whether PAPI features are enabled" on)
option(ENABLE_MKL
       "Specify whether MKL features are enabled" on)
option(ENABLE_BLAS
       "Specify whether BLAS features are enabled" on)
option(ENABLE_LAPACK
       "Specify whether LAPACK features are enabled" on)
option(ENABLE_SCALAPACK
       "Specify whether ScaLAPACK features are enabled" on)
option(ENABLE_PLASMA
       "Specify whether PLASMA features are enabled" on)
option(ENABLE_HDF5
       "Specify whether HDF5 features are enabled" on)

if (BUILD_COVERAGE_TESTS)
  set(BUILD_TESTS TRUE CACHE BOOLEAN
      "Whether tests are built with code coverage (lcov) support.
       Implies BUILD_TESTS. Requires GCC." on)
endif()

include(${CMAKE_SOURCE_DIR}/CMakeExt/CMakeEnvCheck.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/MessageColor.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/InstallFiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/FileSystem.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Doxygen.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Platform.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Environment.cmake)

# Load build modules to locate libraries after environment setup
# has been loaded:
include(${CMAKE_SOURCE_DIR}/CMakeExt/MPI.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/PAPI.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Hwloc.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/Likwid.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/NUMA.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/MKL.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/IPM.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/PLASMA.cmake)
include(${CMAKE_SOURCE_DIR}/CMakeExt/HDF5.cmake)
if (NOT MKL_FOUND)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/LAPACK.cmake)
  include(${CMAKE_SOURCE_DIR}/CMakeExt/SCALAPACK.cmake)
endif()

# prepare StaticConfig.h generation
include(${CMAKE_SOURCE_DIR}/CMakeExt/GenerateConfig.cmake)

## Version number
set(DASH_VERSION_MAJOR 0 CACHE STRING "DASH major version number.")
set(DASH_VERSION_MINOR 3 CACHE STRING "DASH minor version number.")
set(DASH_VERSION_PATCH 0 CACHE STRING "DASH patch version number.")
mark_as_advanced(
  DASH_VERSION_MAJOR
  DASH_VERSION_MINOR
  DASH_VERSION_PATCH)

set(DASH_VERSION
    "${DASH_VERSION_MAJOR}.${DASH_VERSION_MINOR}.${DASH_VERSION_PATCH}"
    CACHE STRING INTERNAL FORCE)
set(DASH_VERSIONED_PROJECT_NAME
    "dash-${DASH_VERSION_MAJOR}.${DASH_VERSION_MINOR}.${DASH_VERSION_PATCH}"
    CACHE STRING INTERNAL FORCE)
set(DART_IF_VERSION "3.2" CACHE STRING
    "Version of the DART interface")

set(CMAKE_RULE_MESSAGES OFF)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_COLOR_MAKEFILE ON)

## Install path
if (INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX ${INSTALL_PREFIX})
else()
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/opt/")
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

## Subprojects, one for each deliverable

set(DART_IMPLEMENTATIONS
    "mpi" CACHE STRING
    "Comma-separated list of DASH runtime implementations to build.
     Default is 'mpi'")

string(REPLACE "," ";"
       DART_IMPLEMENTATIONS_LIST
       ${DART_IMPLEMENTATIONS})

## Include path of selected DART interface:
set(DASH_DART_IF_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-if/v${DART_IF_VERSION}/include
    CACHE PATH INTERNAL FORCE)
set(DASH_DART_BASE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/dart-impl/base/include
    CACHE PATH INTERNAL FORCE)

## Set compiler flags (depend on CMake options)
include(${CMAKE_SOURCE_DIR}/CMakeExt/CompilerFlags.cmake)

## Build results output directories (/bin, /lib, /doc)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shared)

# DART interface:
add_subdirectory(dart-if)

# DART base:
add_subdirectory(dart-impl/base)

# DART implementation of every enabled DART variant:
if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";mpi;")
  set(DART_IMPLEMENTATION_MPI_ENABLED ON
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/mpi)
endif()

if (";${DART_IMPLEMENTATIONS_LIST};" MATCHES ";shmem;")
  set(DART_IMPLEMENTATION_SHMEM_ENABLED ON
      CACHE BOOL INTERNAL FORCE)
  add_subdirectory(dart-impl/shmem)
endif()

add_subdirectory(dash)

## Documentation
if (BUILD_DOCS)
  CreateDoxygenTarget()
endif()

message(EMPH "DASH version ${DASH_VERSION}")
message(EMPH "DART interface version " ${DART_IF_VERSION})
message(EMPH "-----------------------------------------------------------")
message(EMPH "Install prefix:           (INSTALL_PREFIX)                 "
        ${CMAKE_INSTALL_PREFIX})
message(EMPH "Build type:               (CMAKE_BUILD_TYPE)               "
        ${CMAKE_BUILD_TYPE})

message(INFO "Host system identifier:   (ENVIRONMENT_TYPE)               "
        ${DASH_ENV_HOST_SYSTEM_ID})
message(INFO "Strict compiler warnings: (ENABLE_COMPILER_WARNINGS)       "
        ${ENABLE_COMPILER_WARNINGS})
message(INFO "Debug log messages:       (ENABLE_LOGGING)                 "
        ${ENABLE_LOGGING})
message(INFO "Trace log messages:       (ENABLE_TRACE_LOGGING)           "
        ${ENABLE_TRACE_LOGGING})
message(INFO "DART log messages:        (ENABLE_DART_LOGGING)            "
        ${ENABLE_DART_LOGGING})
message(INFO "Runtime assertions:       (ENABLE_ASSERTIONS)              "
        ${ENABLE_ASSERTIONS})
message(INFO "Unified RMA memory model: (ENABLE_UNIFIED_MEMORY_MODEL)    "
        ${ENABLE_UNIFIED_MEMORY_MODEL})
message(INFO "MPI shared windows:       (ENABLE_SHARED_WINDOWS)          "
        ${ENABLE_SHARED_WINDOWS})
message(INFO "Default index type long:  (ENABLE_DEFAULT_INDEX_TYPE_LONG) "
        ${ENABLE_DEFAULT_INDEX_TYPE_LONG})
message(INFO "libnuma support:          (ENABLE_LIBNUMA)                 "
        ${ENABLE_LIBNUMA})
message(INFO "hwloc support:            (ENABLE_HWLOC)                   "
        ${ENABLE_HWLOC})
message(INFO "PAPI support:             (ENABLE_PAPI)                    "
        ${ENABLE_PAPI})
message(INFO "MKL support:              (ENABLE_MKL)                     "
        ${ENABLE_MKL})
message(INFO "BLAS support:             (ENABLE_BLAS)                    "
        ${ENABLE_BLAS})
message(INFO "LAPACK support:           (ENABLE_LAPACK)                  "
        ${ENABLE_LAPACK})
message(INFO "ScaLAPACK support:        (ENABLE_SCALAPACK)               "
        ${ENABLE_SCALAPACK})
message(INFO "PLASMA support:           (ENABLE_PLASMA)                  "
        ${ENABLE_PLASMA})
message(INFO "HDF5 support:             (ENABLE_HDF5)                    "
        ${ENABLE_HDF5})
message(INFO "Enabled DART backends:    (DART_IMPLEMENTATIONS)           "
        ${DART_IMPLEMENTATIONS})
message(INFO "C   compiler id:          ${CMAKE_C_COMPILER_ID}")
message(INFO "C++ compiler id:          ${CMAKE_CXX_COMPILER_ID}")
if (MPI_FOUND)
  message(INFO "MPI implementation:       " ${MPI_IMPL_ID})
endif()

if (ENABLE_PAPI)
  if (PAPI_FOUND)
    message(INFO "PAPI                      enabled")
  else()
    message(NOTE "PAPI                      not found")
  endif()
else()
  message(NOTE "PAPI                      disabled")
endif()
if (ENABLE_MKL)
  if (MKL_FOUND)
    message(INFO "MKL                       enabled")
  else()
    message(NOTE "MKL                       not found")
  endif()
else()
  message(NOTE "MKL                       disabled")
endif()
if (ENABLE_BLAS)
  if (BLAS_FOUND)
    message(INFO "BLAS                      enabled")
  else()
    message(NOTE "BLAS                      not found")
  endif()
else()
  message(NOTE "BLAS                      disabled")
endif()
if (ENABLE_HWLOC)
  if (HWLOC_FOUND)
    message(INFO "hwloc                     enabled")
  else()
    message(NOTE "hwloc                     not found")
  endif()
else()
  message(NOTE "hwloc                     disabled")
endif()
if (ENABLE_LIKWID)
  if (LIKWID_FOUND)
    message(INFO "likwid                    enabled")
  else()
    message(NOTE "likwid                    not found")
  endif()
else()
  message(NOTE "likwid                    disabled")
endif()
if (ENABLE_LAPACK)
  if (LAPACK_FOUND)
    message(INFO "LAPACK                    enabled")
  else()
    message(NOTE "LAPACK                    not found")
  endif()
else()
  message(NOTE "LAPACK                    disabled")
endif()
if (ENABLE_SCALAPACK)
  if (MKL_SCALAPACK_FOUND)
    message(INFO "ScaLAPACK (MKL)           enabled")
  elseif (SCALAPACK_FOUND)
    message(INFO "ScaLAPACK                 enabled")
  else()
    message(NOTE "ScaLAPACK                 not found")
  endif()
else()
  message(NOTE "ScaLAPACK                 disabled")
endif()
if (ENABLE_PLASMA)
  if (PLASMA_FOUND)
    message(INFO "PLASMA                    enabled")
  else()
    message(NOTE "PLASMA                    not found")
  endif()
else()
  message(NOTE "PLASMA                    disabled")
endif()
if (ENABLE_HDF5)
  if (HDF5_FOUND)
    message(INFO "HDF5                      enabled")
  else()
    message(NOTE "HDF5                      not found")
  endif()
else()
  message(NOTE "HDF5                      disabled")
endif()
if (ENABLE_LIBNUMA)
  if (NUMA_FOUND)
    message(INFO "libnuma                   enabled")
  else()
    message(NOTE "libnuma                   not found")
  endif()
else()
  message(NOTE "libnuma                   disabled")
endif()

