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
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
MESSAGE(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(Arena CXX)
ENABLE_TESTING()

option(OLD_POPPLER "Support poppler-lib < 0.83" OFF)
option(AUDIO "Enable AudioCurve OFX" OFF)
option(RICHTEXT "Enable RichText OFX" OFF)
option(HALDCLUT "Enable HaldCLUT OFX (requires curl)" OFF)
option(BUNDLE_FONTS_CONF "Bundle fonts.conf" OFF)
option(READMISC_LEGACY "Enable legacy file formats in ReadMisc" OFF)
set(MAGICK_PKG_CONFIG "Magick++" CACHE STRING "ImageMagick pkg-config name")

# Require OpenGL
if(POLICY CMP0072)
    # prefer GLVND ABI. We use the compatibility via OPENGL_LIBRARIES variable
    # see https://cmake.org/cmake/help/v3.11/policy/CMP0072.html
    # see https://cmake.org/cmake/help/v3.11/module/FindOpenGL.html#linux-specific
    cmake_policy(SET CMP0072 NEW)
endif()
include(FindOpenGL REQUIRED)

# not super portable, but will do for now:
find_package(PkgConfig REQUIRED)
pkg_search_module(MAGICK REQUIRED ${MAGICK_PKG_CONFIG})
pkg_search_module(LCMS2 REQUIRED lcms2)
pkg_search_module(FONTCONFIG REQUIRED fontconfig)
pkg_search_module(PANGO REQUIRED pangocairo)
pkg_search_module(XML REQUIRED libxml-2.0)
pkg_search_module(ZIP REQUIRED libzip)
pkg_search_module(SVG REQUIRED librsvg-2.0)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(CAIRO REQUIRED cairo)
pkg_search_module(POPPLER REQUIRED poppler-glib)
pkg_check_modules(REVENGE REQUIRED librevenge-0.0 librevenge-generators-0.0 librevenge-stream-0.0)
pkg_search_module(CDR REQUIRED libcdr-0.1)
add_compile_options(${MAGICK_CFLAGS}) # we need the magick flags

if(${READMISC_LEGACY})
    add_definitions(-DREADMISC_LEGACY)
endif()

if(${OLD_POPPLER})
    add_definitions(-DLEGACY_POPPLER)
endif()

if(${AUDIO})
    pkg_search_module(SOX REQUIRED sox)
    add_compile_options(${SOX_CFLAGS}) # we need the sox flags
endif()

if(${HALDCLUT})
    # HaldCLUT uses libCurl to download ~100MiB of CLUT profiles
    pkg_search_module(CURL REQUIRED libcurl)
endif()

set (CMAKE_CXX_STANDARD 11)

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
SET(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/OpenFX)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/OpenFX/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# C++ Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/SupportExt)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/Plugins/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/OpenFX-IO/IOSupport)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/OpenFX-IO/IOSupport/SequenceParsing)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/OpenFX-IO/IOSupport/SequenceParsing/tinydir)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Magick)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Extra)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Audio)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Bundle)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/Text)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lodepng)
#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/OCL)

# Define "DEBUG" on debug builds
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# /MP on MSVC
#ADD_COMPILE_OPTIONS($<$<CXX_COMPILER_ID:MSVC>:/MP>)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(OFX_SUPPORT_HEADER_DIR "${OPENFX_PATH}/Support/include")
set(OFX_SUPPORT_LIBRARY_DIR "${OPENFX_PATH}/Support/Library")

FILE(GLOB SUPPORT_SOURCES
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
    "SupportExt/ofxsOGLTextRenderer.cpp"
    "SupportExt/ofxsOGLFontData.cpp"
    "SupportExt/ofxsRectangleInteract.cpp"
    "SupportExt/ofxsTransform3x3.cpp"
    "SupportExt/ofxsTransformInteract.cpp"
    "SupportExt/ofxsShutter.cpp"
    "SupportExt/ofxsMultiPlane.cpp"
    "OpenFX-IO/IOSupport/GenericReader.cpp"
    "OpenFX-IO/IOSupport/GenericOCIO.cpp"
    "OpenFX-IO/IOSupport/SequenceParsing/SequenceParsing.cpp"
    "SupportExt/*.h"
)

FILE(GLOB SOURCES
    "Text/TextOFX.cpp"
    "Text/RichText.cpp"
    "Extra/ReadSVG.cpp"
    "Extra/ReadPDF.cpp"
    "Extra/OpenRaster.cpp"
    "Extra/ReadCDR.cpp"
    "Extra/ReadKrita.cpp"
    "Magick/Arc.cpp"
    "Magick/Charcoal.cpp"
    "Magick/Edges.cpp"
    "Magick/Implode.cpp"
    "Magick/Modulate.cpp"
    "Magick/Oilpaint.cpp"
    "Magick/Polar.cpp"
    "Magick/Polaroid.cpp"
    "Magick/ReadMisc.cpp"
    "Magick/ReadPSD.cpp"
    "Magick/Reflection.cpp"
    "Magick/Sketch.cpp"
    "Magick/Text.cpp"
    "Magick/Texture.cpp"
    "Magick/Tile.cpp"
    "Magick/Roll/Roll.cpp"
    "Magick/Swirl/Swirl.cpp"
    "Magick/Wave/Wave.cpp"
    "Magick/Morphology/Morphology.cpp"
    "Magick/MagickPlugin.cpp"
    "lodepng/lodepng.cpp"
)

FILE(GLOB RESOURCES
    "Extra/net.fxarena.openfx.*.png"
    "Extra/net.fxarena.openfx.*.svg"
    "Extra/fr.inria.openfx.*.png"
    "Extra/fr.inria.openfx.*.svg"
    "Text/net.fxarena.openfx.*.png"
    "Text/net.fxarena.openfx.*.svg"
    "Magick/net.fxarena.openfx.*.png"
    "Magick/net.fxarena.openfx.*.svg"
    "Magick/Roll/net.fxarena.openfx.*.png"
    "Magick/Roll/net.fxarena.openfx.*.svg"
    "Magick/Swirl/net.fxarena.openfx.*.png"
    "Magick/Swirl/net.fxarena.openfx.*.svg"
    "Magick/Wave/net.fxarena.openfx.*.png"
    "Magick/Wave/net.fxarena.openfx.*.svg"
    "Magick/Morphology/net.fxarena.openfx.*.png"
    "Magick/Morphology/net.fxarena.openfx.*.svg"
    "Magick/Extra.png"
    "Magick/Extra.svg"
)

if(${AUDIO})
    list(APPEND SOURCES "Audio/AudioCurve.cpp")
    list(APPEND RESOURCES "Audio/net.fxarena.openfx.AudioCurve.png")
endif()

if(${RICHTEXT})
    list(APPEND SOURCES "Text/RichTextOFX.cpp")
    list(
        APPEND
        RESOURCES
        "Text/net.fxarena.openfx.RichText.png"
        "Text/net.fxarena.openfx.RichText.svg")
endif()

if(${HALDCLUT})
    list(APPEND SOURCES "Magick/HaldCLUT/HaldCLUT.cpp")
    list(
        APPEND
        RESOURCES
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.png"
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.svg"
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.xml")
endif()

# fonts.conf
if (APPLE)
    set(FC_FONTS_DIR "<dir>/System/Library/Fonts</dir><dir>/Library/Fonts</dir><dir>~/Library/Fonts</dir>")
    set(FC_CACHE_DIR "<cachedir>~/.cache/fontconfig</cachedir>")
elseif(WIN32)
    set(FC_FONTS_DIR "<dir>WINDOWSFONTDIR</dir>")
    set(FC_CACHE_DIR "<cachedir>LOCAL_APPDATA_FONTCONFIG_CACHE</cachedir>")
else()
    set(FC_FONTS_DIR "<dir>/usr/share/fonts/OTF</dir><dir>/usr/share/fonts/TTF</dir><dir>/usr/share/fonts/Type1</dir><dir>/usr/share/fonts/Speedo</dir><dir>/usr/share/fonts/cyrillic</dir><dir>/usr/share/fonts/misc</dir><dir prefix=\"xdg\">fonts</dir>")
    set(FC_CACHE_DIR "<cachedir prefix=\"xdg\">fontconfig</cachedir><cachedir>~/.fontconfig</cachedir>")
endif()
configure_file(
${CMAKE_CURRENT_SOURCE_DIR}/Text/fonts.conf.in
${CMAKE_CURRENT_BINARY_DIR}/fonts.conf
@ONLY)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCES} ${SUPPORT_SOURCES})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-fvisibility=hidden -exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-shared -fvisibility=hidden -Xlinker --version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-fvisibility=hidden,--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
endif()

TARGET_COMPILE_DEFINITIONS(
    ${PROJECT_NAME}
    PRIVATE
    OFX_EXTENSIONS_VEGAS
    OFX_EXTENSIONS_NUKE
    OFX_EXTENSIONS_NATRON
    OFX_EXTENSIONS_TUTTLE
    OFX_SUPPORTS_OPENGLRENDER
    NOMINMAX
)
# use compatibiltiy/legacy ABI also on systems based on GLVND
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OPENGL_LIBRARIES})
#TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OPENGL_gl_LIBRARY})

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
    ${MAGICK_LIBRARIES}
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
    ${CDR_LDFLAGS}
)

if(${AUDIO})
    include_directories(${SOX_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SOX_LDFLAGS})
endif()

if(${HALDCLUT})
    include_directories(${CURL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${CURL_LDFLAGS})
endif()

#IF (MSVC)
#  # Some files require this option. This breaks compatibility with older linkers.
#  SET_TARGET_PROPERTIES(Misc PROPERTIES COMPILE_FLAGS "/bigobj")
#ENDIF ()

# Find and set the arch name.
# http://openeffects.org/documentation/reference/ch02s02.html
SET(OFX_ARCH UNKNOWN)
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

SET(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
IF("${OFX_ARCH_NAME}" EQUAL "UNKNOWN")
  MESSAGE(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
ELSE()
  INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )
  INSTALL(FILES ${RESOURCES}
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources
  )
  INSTALL(FILES Bundle/Info.plist
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents
  )
  if(${BUNDLE_FONTS_CONF})
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/fonts.conf
        DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources/fonts
    )
  endif()
ENDIF()
