CMAKE_MINIMUM_REQUIRED ( VERSION 2.8 )

PROJECT ( PBRT-V3 )

ENABLE_TESTING()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/openexr/OpenEXR")
  message(FATAL_ERROR "The OpenEXR submodule directory is missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by calling \"git submodule update --init --recursive\"")
endif()

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register")
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-conversion-null")
  ENDIF()
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

  FIND_PROGRAM(XIAR xiar)
  IF(XIAR)
    SET(CMAKE_AR "${XIAR}")
  ENDIF(XIAR)
  MARK_AS_ADVANCED(XIAR)

  FIND_PROGRAM(XILD xild)
  IF(XILD)
    SET(CMAKE_LINKER "${XILD}")
  ENDIF(XILD)
  MARK_AS_ADVANCED(XILD)

  # ICC will default to -fp-model fast=1, which performs value-unsafe optimizations which will
  # cause pbrt_test to fail. For safety, -fp-model precise is explicitly set here by default.
  set(FP_MODEL "precise" CACHE STRING "The floating point model to compile with.")
  set_property(CACHE FP_MODEL PROPERTY STRINGS "precise" "fast=1" "fast=2")

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fp-model ${FP_MODEL}")
ENDIF()

IF(MSVC)
  ADD_DEFINITIONS (/D _CRT_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS (/D YY_NO_UNISTD_H)
ENDIF()

FIND_PACKAGE ( Threads )

IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
  ADD_DEFINITIONS (-DNDEBUG)
ENDIF()

# Optionally use Bison and Flex to regenerate parser files
# Use pregenerated files otherwise (may be outdated)
FIND_PACKAGE ( BISON )
FIND_PACKAGE ( FLEX )
IF(BISON_FOUND AND FLEX_FOUND)
  SET(BisonOutput ${CMAKE_BINARY_DIR}/pbrtparse.cpp)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${BisonOutput} ${CMAKE_BINARY_DIR}/pbrtparse.hh
    DEPENDS ${CMAKE_SOURCE_DIR}/src/core/pbrtparse.yy
    COMMAND ${BISON_EXECUTABLE} -d -v -t
            --output=${BisonOutput}
            ${CMAKE_SOURCE_DIR}/src/core/pbrtparse.yy
    COMMAND bash -c "if [[ -e ${CMAKE_BINARY_DIR}/pbrtparse.hpp ]]\; then /bin/mv ${CMAKE_BINARY_DIR}/pbrtparse.hpp ${CMAKE_BINARY_DIR}/pbrtparse.hh\; fi"
    COMMENT "Generating pbrtparse.cpp and pbrtparse.hh"
  )

  SET(FlexOutput ${CMAKE_BINARY_DIR}/pbrtlex.cpp)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${FlexOutput}
    DEPENDS ${CMAKE_BINARY_DIR}/pbrtparse.hh ${CMAKE_SOURCE_DIR}/src/core/pbrtlex.ll
    COMMAND ${FLEX_EXECUTABLE}
            -o${FlexOutput}
            ${CMAKE_SOURCE_DIR}/src/core/pbrtlex.ll
    COMMENT "Generating pbrtlex.cpp"
  )

  SET ( PBRT_YACC_LEX_SOURCE
    ${BisonOutput}
    ${FlexOutput}
    )
ELSE()
  SET ( PBRT_YACC_LEX_SOURCE
    src/core/pbrtlex.cpp
    src/core/pbrtparse.cpp
    )
ENDIF()

IF(WIN32)
  # Build zlib (only on Windows)
  SET(ZLIB_BUILD_STATIC_LIBS ON CACHE BOOL " " FORCE)
  SET(ZLIB_BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
  ADD_SUBDIRECTORY(src/ext/zlib)

  SET(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/zlib")
  SET(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/src/ext/zlib/$<CONFIGURATION>/zlibstatic.lib")
  SET_PROPERTY(TARGET zlibstatic PROPERTY FOLDER "dependencies")
  INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/src/ext/zlib")
ENDIF()

SET(ILMBASE_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
SET(OPENEXR_NAMESPACE_VERSIONING OFF CACHE BOOL " " FORCE)
SET(OPENEXR_BUILD_SHARED_LIBS    OFF CACHE BOOL " " FORCE)
SET(ILMBASE_BUILD_SHARED_LIBS    OFF CACHE BOOL " " FORCE)
ADD_SUBDIRECTORY(src/ext/openexr)
SET_PROPERTY(TARGET IexMath eLut toFloat b44ExpLogTable dwaLookups CopyIlmBaseLibs IlmThread Half Iex Imath IlmImf PROPERTY FOLDER "dependencies")
INCLUDE_DIRECTORIES (
  src/ext/openexr/IlmBase/Imath
  src/ext/openexr/IlmBase/Half
  src/ext/openexr/IlmBase/Iex
  src/ext/openexr/OpenEXR/IlmImf
  ${CMAKE_BINARY_DIR}/src/ext/openexr/IlmBase/config
  ${CMAKE_BINARY_DIR}/src/ext/openexr/OpenEXR/config
)

SET(OPENEXR_LIBS IlmImf Imath Half)

SET(EMBREE_INCLUDE_DIR " " CACHE "Embree2 include directory" PATH)
SET(EMBREE_LIBRARY     " " CACHE "Embree2 library file" FILEPATH)
INCLUDE_DIRECTORIES(${EMBREE_INCLUDE_DIR})


SET(ALTA_INCLUDE_DIR " " CACHE "ALTA include directory" PATH)
SET(ALTA_LIBRARY     " " CACHE "ALTA core library file" FILEPATH)
INCLUDE_DIRECTORIES(${ALTA_INCLUDE_DIR})

SET ( PBRT_CORE_SOURCE
  src/core/api.cpp
  src/core/bssrdf.cpp
  src/core/camera.cpp
  src/core/efloat.cpp
  src/core/error.cpp
  src/core/fileutil.cpp
  src/core/film.cpp
  src/core/filter.cpp
  src/core/floatfile.cpp
  src/core/geometry.cpp
  src/core/imageio.cpp
  src/core/integrator.cpp
  src/core/interaction.cpp
  src/core/interpolation.cpp
  src/core/light.cpp
  src/core/lowdiscrepancy.cpp
  src/core/material.cpp
  src/core/medium.cpp
  src/core/memory.cpp
  src/core/microfacet.cpp
  src/core/parallel.cpp
  src/core/paramset.cpp
  src/core/parser.cpp
  src/core/primitive.cpp
  src/core/progressreporter.cpp
  src/core/quaternion.cpp
  src/core/reflection.cpp
  src/core/sampler.cpp
  src/core/sampling.cpp
  src/core/scene.cpp
  src/core/shape.cpp
  src/core/sobolmatrices.cpp
  src/core/spectrum.cpp
  src/core/stats.cpp
  src/core/texture.cpp
  src/core/transform.cpp
  )

SET ( PBRT_CORE_HEADERS
  src/core/api.h
  src/core/bssrdf.h
  src/core/camera.h
  src/core/efloat.h
  src/core/error.h
  src/core/fileutil.h
  src/core/film.h
  src/core/filter.h
  src/core/floatfile.h
  src/core/geometry.h
  src/core/imageio.h
  src/core/integrator.h
  src/core/interaction.h
  src/core/interpolation.h
  src/core/light.h
  src/core/lowdiscrepancy.h
  src/core/material.h
  src/core/medium.h
  src/core/memory.h
  src/core/microfacet.h
  src/core/mipmap.h
  src/core/parallel.h
  src/core/paramset.h
  src/core/parser.h
  src/core/pbrt.h
  src/core/primitive.h
  src/core/progressreporter.h
  src/core/quaternion.h
  src/core/reflection.h
  src/core/rng.h
  src/core/sampler.h
  src/core/sampling.h
  src/core/scene.h
  src/core/shape.h
  src/core/sobolmatrices.h
  src/core/spectrum.h
  src/core/stats.h
  src/core/stdafx.h
  src/core/texture.h
  src/core/transform.h
  )

FILE ( GLOB PBRT_SOURCE
  src/ext/*.cpp
  src/accelerators/*.cpp
  src/cameras/*.cpp
  src/filters/*.cpp
  src/integrators/*.cpp
  src/lights/*.cpp
  src/materials/*.cpp
  src/samplers/*.cpp
  src/shapes/*.cpp
  src/textures/*.cpp
  src/media/*.cpp
  )

INCLUDE_DIRECTORIES ( src )
INCLUDE_DIRECTORIES ( src/core )

SET(EIGEN3_INCLUDE_DIR " " CACHE "Eigen3 include directory" FILEPATH)
IF( EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Core" )
   include_directories( ${EIGEN3_INCLUDE_DIR} )
ELSE()
   MESSAGE("COULD NOT FIND EIGEN3 CORE HEADERS: ALTA WILL NOT BE COMPILED")
ENDIF()

ADD_LIBRARY ( pbrt STATIC
  ${PBRT_YACC_LEX_SOURCE}
  ${PBRT_CORE_SOURCE}
  ${PBRT_CORE_HEADERS}
  ${PBRT_SOURCE}
  )

IF( (EXISTS "${ALTA_INCLUDE_DIR}/core/plugins_manager.h") AND 
   (EXISTS ${ALTA_LIBRARY}) AND (EXISTS "${EIGEN3_INCLUDE_DIR}/Eigen/Core") )
   MESSAGE("USING ALTA LIBRARY")
   target_compile_definitions( pbrt PRIVATE USE_ALTA=1 )
ELSE()
   MESSAGE("COULD NOT FIND ALTA LIBRARY (HEADERS OR LIB FILE)")
ENDIF()

IF (WIN32)
  # Avoid a name clash when building on Visual Studio
  SET_TARGET_PROPERTIES ( pbrt
    PROPERTIES
    OUTPUT_NAME libpbrt
    )
ENDIF()

TARGET_LINK_LIBRARIES ( pbrt
  ${EMBREE_LIBRARY}
  ${ALTA_LIBRARY}
  )

FIND_PACKAGE ( Threads )

# Main renderer
ADD_EXECUTABLE ( pbrt_exe
  src/main/pbrt.cpp
  )

SET_TARGET_PROPERTIES ( pbrt_exe
  PROPERTIES
  OUTPUT_NAME pbrt
  )

TARGET_LINK_LIBRARIES ( pbrt_exe
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

# Tools
ADD_EXECUTABLE ( bsdftest
  src/tools/bsdftest.cpp
  )


ADD_EXECUTABLE ( exrdiff
  src/tools/exrdiff.cpp
  )

ADD_EXECUTABLE ( exravg
  src/tools/exravg.cpp
  )

ADD_EXECUTABLE ( hdrtoldr
  src/tools/hdrtoldr.cpp
  )

ADD_EXECUTABLE ( obj2pbrt
  src/tools/obj2pbrt.cpp
  )

TARGET_LINK_LIBRARIES ( bsdftest
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

TARGET_LINK_LIBRARIES ( exrdiff
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

TARGET_LINK_LIBRARIES ( exravg
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

TARGET_LINK_LIBRARIES ( hdrtoldr
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

TARGET_LINK_LIBRARIES ( obj2pbrt
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  )

# Unit test

FILE ( GLOB PBRT_TEST_SOURCE
  src/tests/*.cpp
  src/tests/gtest/*.cc
  )

ADD_EXECUTABLE ( pbrt_test
  ${PBRT_TEST_SOURCE}
  )

TARGET_LINK_LIBRARIES ( pbrt_test
  pbrt
  ${CMAKE_THREAD_LIBS_INIT}
  ${OPENEXR_LIBS}
  )

ADD_TEST ( pbrt_unit_test
  pbrt_test
  )

# Installation

INSTALL ( TARGETS
  pbrt_exe
  bsdftest
  exrdiff
  hdrtoldr
  obj2pbrt
  DESTINATION
  bin
  )

INSTALL ( TARGETS
  pbrt
  DESTINATION
  lib
  )
