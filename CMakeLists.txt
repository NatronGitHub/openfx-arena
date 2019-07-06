# ***** BEGIN LICENSE BLOCK *****
# Taken from openfx-misc and modified for openfx-arena
# Copyright (C) 2013-2018 INRIA
#
# openfx-misc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# openfx-misc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with openfx-misc.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

########################################################################
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
message(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 3.1)
project(Arena CXX)
enable_testing()

include(FindOpenGL REQUIRED)

# not super portable, but will do for now:
find_package(PkgConfig REQUIRED)
pkg_search_module(MAGICK REQUIRED Magick++)
pkg_search_module(LCMS2 REQUIRED lcms2)
pkg_search_module(FONTCONFIG REQUIRED fontconfig)
pkg_search_module(PANGO REQUIRED pangocairo)
pkg_search_module(XML REQUIRED libxml-2.0)
pkg_search_module(ZIP REQUIRED libzip)
pkg_search_module(SVG REQUIRED librsvg-2.0)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(CAIRO REQUIRED cairo)
pkg_search_module(POPPLER REQUIRED poppler-glib)
pkg_search_module(REVENGE REQUIRED librevenge-0.0)
pkg_search_module(CDR REQUIRED libcdr-0.1)
add_compile_options(${MAGICK_CFLAGS}) # we need the magick flags

set(CMAKE_CXX_STANDARD 11)

# Use -Ofast when applicable (implies -ffast-math)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Ofast -DNDEBUG")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -Ofast -DNDEBUG")
    set (CMAKE_C_FLAGS_RELEASE   "-Wall -Wextra -Ofast -DNDEBUG")
endif()
set(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/common/OpenFX/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i --recursive\n")
endif()

# C++ Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt)
include_directories(${OPENFX_PATH}/include)
include_directories(${OPENFX_PATH}/Support/include)
include_directories(${OPENFX_PATH}/Support/Plugins/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport/SequenceParsing)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport/SequenceParsing/tinydir)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common/lodepng)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/common/OpenCL)

# Define "DEBUG" on debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# /MP on MSVC
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")

file(GLOB SUPPORT_SOURCES
    "${OFX_SUPPORT_HEADER_DIR}/*.h"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsOGLTextRenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsOGLFontData.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsRectangleInteract.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsTransform3x3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsTransformInteract.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsShutter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsMultiPlane.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport/GenericReader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport/GenericOCIO.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/OpenFX-IO/IOSupport/SequenceParsing/SequenceParsing.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/lodepng"
)

file(GLOB SUPPORT_SOURCES_CL
    "${OFX_SUPPORT_HEADER_DIR}/*.h"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsCore.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsImageEffect.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsInteract.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsLog.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsMultiThread.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsParams.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsProperty.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsPropertyValidation.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsTransform3x3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ofxsTransformInteractCustom.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsShutter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/ofxsMultiPlane.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/SupportExt/*.h"
)

file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Text.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/TextFX.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadSVG.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadPDF.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/OpenRaster.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadCDR.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadKrita.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Arc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Charcoal.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Edges.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Modulate.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Oilpaint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Polar.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Polaroid.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadMisc.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ReadPSD.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Reflection.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Sketch.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Texture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick/Tile.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Roll.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Swirl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Wave.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Implode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MagickPlugin.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/common/lodepng/lodepng.cpp"
)

FILE(GLOB SOURCES_CL
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bulge.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Duotone.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Ripple.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Twirl.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Vibrance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Bloom.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/CLFilter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/OCLPlugin.cpp"
)

file(GLOB RESOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/fr.inria.openfx.*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/fr.inria.openfx.*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/net.fxarena.openfx.*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/net.fxarena.openfx.*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/Arena/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/Arena/*.svg"
)

file(GLOB RESOURCES_CL
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/net.fxarena.opencl.*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/images/net.fxarena.opencl.*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/kernels/net.fxarena.opencl.*.cl"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/Arena/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/Arena/*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/ArenaCL/*.png"
)

# Arena.ofx
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${SUPPORT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".ofx")

# ArenaCL.ofx
add_library(${PROJECT_NAME}CL SHARED ${SOURCES_CL} ${SUPPORT_SOURCES_CL})
set_target_properties(${PROJECT_NAME}CL PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME}CL PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
    set_target_properties(${PROJECT_NAME}CL PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(${PROJECT_NAME}CL PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(${PROJECT_NAME}CL PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
    set_target_properties(${PROJECT_NAME}CL PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(${PROJECT_NAME}CL PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
    OFX_EXTENSIONS_VEGAS
    OFX_EXTENSIONS_NUKE
    OFX_EXTENSIONS_NATRON
    OFX_EXTENSIONS_TUTTLE
    OFX_SUPPORTS_OPENGLRENDER
    NOMINMAX
)

target_compile_definitions(
    ${PROJECT_NAME}CL
    PRIVATE
    OFX_EXTENSIONS_VEGAS
    OFX_EXTENSIONS_NUKE
    OFX_EXTENSIONS_NATRON
    OFX_EXTENSIONS_TUTTLE
    OFX_SUPPORTS_OPENGLRENDER
    NOMINMAX
)

include_directories(
    ${MAGICK_INCLUDE_DIRS}
    ${LCMS2_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${PANGO_INCLUDE_DIRS}
    ${XML_INCLUDE_DIRS}
    ${ZIP_INCLUDE_DIRS}
    ${SVG_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${POPPLER_INCLUDE_DIRS}
    ${REVENGE_INCLUDE_DIRS}
    ${CDR_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    ${OPENGL_gl_LIBRARY}
    ${MAGICK_STATIC_LIBRARIES}
    ${LCMS2_LIBRARIES}
    ${MAGICK_LDFLAGS}
    ${LCMS2_LDFLAGS}
    ${FONTCONFIG_LDFLAGS}
    ${PANGO_LDFLAGS}
    ${XML_LDFLAGS}
    ${ZIP_LDFLAGS}
    ${SVG_LDFLAGS}
    ${GLIB_LDFLAGS}
    ${CAIRO_LDFLAGS}
    ${POPPLER_LDFLAGS}
    ${REVENGE_LDFLAGS}
    ${REVENGE_LDFLAGS}
)
target_link_libraries(${PROJECT_NAME}CL ${OPENGL_gl_LIBRARY})

# OpenCL
if((APPLE) OR (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD"))
    find_package(OpenCL REQUIRED)
    #link_directories(${OpenCL_LIBRARY})
    target_link_libraries (${PROJECT_NAME}CL ${OpenCL_LIBRARY})
else()
    # on Windows(MINGW)/Linux use dummy library for linking
    set(BIT "32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(BIT "64")
    endif()
    target_link_libraries(${PROJECT_NAME}CL -L${CMAKE_CURRENT_SOURCE_DIR}/common/OpenCL/lib${BIT} -lOpenCL)
endif()

if(MSVC)
  # Some files require this option. This breaks compatibility with older linkers.
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/bigobj")
endif()

# Find and set the arch name.
# http://openeffects.org/documentation/reference/ch02s02.html
set(OFX_ARCH UNKNOWN)
# Win
if(MINGW)
    set(OFX_ARCH "Win32")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Win64")
    endif()
# Visual Studio    
elseif(MSVC)
    set(OFX_ARCH "Win32")
    if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL "IA64") OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64"))
        set(OFX_ARCH "Win64")
    endif()
# FreeBSD
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(OFX_ARCH "FreeBSD-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "FreeBSD-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH FreeBSD-arm-64)
        else()
            set(OFX_ARCH FreeBSD-arm)
        endif()
    endif()
# MacOS
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(OFX_ARCH "MacOS")
# Linux
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(OFX_ARCH "Linux-x86")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        set(OFX_ARCH "Linux-x86-64")
    endif()
    if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            set(OFX_ARCH Linux-arm-64)
        else()
            set(OFX_ARCH Linux-arm)
        endif()
    endif()
endif()

set(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
if("${OFX_ARCH_NAME}" EQUAL "UNKNOWN")
  message(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
else()
  install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )
  install(
    TARGETS ${PROJECT_NAME}CL
    RUNTIME DESTINATION ${PROJECT_NAME}CL.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION ${PROJECT_NAME}CL.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )
  install(
    FILES ${RESOURCES}
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources
  )
  install(
    FILES ${RESOURCES_CL}
    DESTINATION ${PROJECT_NAME}CL.ofx.bundle/Contents/Resources
  )
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/share/Arena/Info.plist
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents
  )
  install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/share/ArenaCL/Info.plist
    DESTINATION ${PROJECT_NAME}CL.ofx.bundle/Contents
  )
endif()
