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
# Project setup
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.16) # as Natron and openfx-io
PROJECT(Text CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
endif()
ENABLE_TESTING()

option(RICHTEXT "Enable RichText OFX" OFF)
option(BUNDLE_FONTS_CONF "Bundle fonts.conf" OFF)

# libGL, as Makefile.master: with GLVND's libOpenGL, OPENGL_gl_LIBRARY is empty and the
# module doesn't load on Linux (undefined symbol: glMultMatrixd)
if(NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(FONTCONFIG REQUIRED fontconfig)
pkg_search_module(PANGO REQUIRED pangocairo)

# C++17 by default (Natron RB-2.6 Qt5 builds); callers may ask for C++20
# (Qt6 builds) with -DCMAKE_CXX_STANDARD=20. The code must build with both.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

# Release: CMake's default -O3 -DNDEBUG, not -Ofast: -ffast-math's -ffinite-math-only
# folds std::isnan/isfinite to constants and lets NaN through std::max (Natron#1277)
SET(OPENFX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../OpenFX)

# Check that submodule have been initialized and updated
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/../OpenFX/include)
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update -i\n")
endif()

# C++ Include directories
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../Magick) # PackedImage.h
INCLUDE_DIRECTORIES(${OPENFX_PATH}/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/include)
INCLUDE_DIRECTORIES(${OPENFX_PATH}/Support/Plugins/include)

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
    # DrawArgs::isOpenGLContextCurrent() and the OpenGL draw context, used by SupportExt's
    # overlays with OFXS_HAVE_DRAWSUITE (it loads OpenGL at run time: needs CMAKE_DL_LIBS)
    "${OFX_SUPPORT_LIBRARY_DIR}/ofxsDrawContextOpenGL.cpp"
    "${OFX_SUPPORT_LIBRARY_DIR}/*.h"
)

FILE(GLOB SOURCES
    "TextOFX.cpp"
    "RichText.cpp"
)

FILE(GLOB RESOURCES
    "net.fxarena.openfx.Text.png"
    "net.fxarena.openfx.Text.svg"
)

if(${RICHTEXT})
    list(APPEND SOURCES "RichTextOFX.cpp")
    list(
        APPEND
        RESOURCES
        "net.fxarena.openfx.RichText.png"
        "net.fxarena.openfx.RichText.svg")
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
${CMAKE_CURRENT_SOURCE_DIR}/fonts.conf.in
${CMAKE_CURRENT_BINARY_DIR}/fonts.conf
@ONLY)

# MODULE: -bundle on macOS, no SONAME elsewhere, as Makefile.master
ADD_LIBRARY(${PROJECT_NAME} MODULE ${SOURCES} ${SUPPORT_SOURCES})
# the SupportExt sources it had (TextOFX's transform interact, MultiPlane), built with
# the plug-in's definitions
include(${CMAKE_CURRENT_SOURCE_DIR}/../SupportExt/cmake/SupportExt.cmake)
supportext_add_sources(${PROJECT_NAME} COMPONENTS TRANSFORM3X3 TRANSFORM_INTERACT MULTIPLANE)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SUFFIX ".ofx")

# Add extra flags to the link step of the plugin: export only OfxGetPlugin and
# OfxGetNumberOfPlugins (the macOS list was given to the compiler driver without
# -Wl, so the link failed, #67; the Linux flags gave -fvisibility=hidden to the linker)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
if(APPLE)
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-exported_symbols_list,${OFX_SUPPORT_HEADER_DIR}/osxSymbols")
    set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
elseif(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
# Linux & FreeBSD
    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--version-script=${OFX_SUPPORT_HEADER_DIR}/linuxSymbols")
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
    # as the root CMakeLists.txt and Makefile.master: SupportExt's DrawSuite overlays (V2), with
    # the same value in every translation unit (ofxsOverlayDraw.h's #pragma detect_mismatch)
    OFXS_HAVE_DRAWSUITE=1
    NOMINMAX
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} OpenGL::GL)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_DL_LIBS})

include_directories(
    ${FONTCONFIG_INCLUDE_DIRS}
    ${PANGO_INCLUDE_DIRS}
)
# pkg-config --static --libs, as Makefile.master; "-framework X" must stay one item
set(_libs "")
set(_framework FALSE)
foreach(_f ${FONTCONFIG_STATIC_LDFLAGS} ${PANGO_STATIC_LDFLAGS})
    if(_framework)
        list(APPEND _libs "-Wl,-framework,${_f}")
        set(_framework FALSE)
    elseif(_f STREQUAL "-framework")
        set(_framework TRUE)
    else()
        list(APPEND _libs "${_f}")
    endif()
endforeach()
target_link_libraries(${PROJECT_NAME} ${_libs})

#IF (MSVC)
#  # Some files require this option. This breaks compatibility with older linkers.
#  SET_TARGET_PROPERTIES(Misc PROPERTIES COMPILE_FLAGS "/bigobj")
#ENDIF ()

# Bundle architecture folder: same rules as openfx cmake/OpenFX.cmake (ARCHDIR), which must
# match what HostSupport looks for (ofxhArchDirs.h) and Support/Plugins/Makefile.master.
if(APPLE)
  set(ARCHDIR "MacOS") # universal or arch-specific: no arm64 folder
elseif(WIN32)
  # MSVC: the target architecture (ARM64EC, ARM64, x64, X86); other compilers: pointer size
  string(TOUPPER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID};${CMAKE_GENERATOR_PLATFORM}" _OFX_WINARCH)
  if(NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    # MinGW (MSYS2 CLANGARM64, llvm-mingw) on CMake < 4.1: the architecture ID is only set for MSVC, and
    # CMAKE_SYSTEM_PROCESSOR is the build machine's unless a toolchain file sets it: ask the compiler for
    # its target (CMAKE_CXX_COMPILER_TARGET, or `-dumpmachine`), e.g. aarch64-w64-mingw32 or
    # arm64ec-w64-windows-gnu (llvm-mingw Arm64EC)
    set(_OFX_TARGET "${CMAKE_CXX_COMPILER_TARGET}")
    if(NOT _OFX_TARGET)
      # CMAKE_CXX_COMPILER_ARG1 is one string (e.g. " --target=aarch64-w64-mingw32"): split it
      # (UNIX_COMMAND works with any CMake version, for projects that copy this block)
      separate_arguments(_OFX_CXX_ARGS UNIX_COMMAND "${CMAKE_CXX_COMPILER_ARG1}")
      execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${_OFX_CXX_ARGS} -dumpmachine
                      OUTPUT_VARIABLE _OFX_TARGET OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    string(TOLOWER "${_OFX_TARGET}" _OFX_TARGET)
    if(_OFX_TARGET MATCHES "^arm64ec")
      set(_OFX_WINARCH "ARM64EC")
    elseif(_OFX_TARGET MATCHES "^(aarch64|arm64)")
      set(_OFX_WINARCH "ARM64")
    endif()
  endif()
  if(_OFX_WINARCH MATCHES "ARM64EC" OR CMAKE_CXX_FLAGS MATCHES "/arm64EC")
    set(ARCHDIR "Win-arm64ec") # normative (upstream main, 2026): Arm64EC (and Arm64X) plug-ins
  elseif(_OFX_WINARCH MATCHES "ARM64|AARCH64")
    # CMake >= 4.1 reports "aarch64" as the MinGW architecture ID
    set(ARCHDIR "Win-arm64")   # reserved, not yet normative: native Arm64 hosts
    message(WARNING "Native Arm64 plug-in: installed to Win-arm64, a reserved, not yet normative folder. "
                    "Current Windows-on-Arm hosts (Resolve, Vegas) are Arm64EC: build for Arm64EC for them.")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCHDIR "Win64")
  else()
    set(ARCHDIR "Win32")
  endif()
elseif(UNIX)
  if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(_OFX_OS "FreeBSD")
  else()
    set(_OFX_OS "Linux")
  endif()
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _OFX_CPU)
  if(_OFX_CPU MATCHES "^(x86_64|amd64|i[3-6]86|x86)$")
    # x86: use the pointer size, the userland may be 32-bit on a 64-bit kernel
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(ARCHDIR "${_OFX_OS}-x86-64")
    else()
      set(ARCHDIR "${_OFX_OS}-x86")
    endif()
  elseif(_OFX_OS STREQUAL "Linux")
    # the spec's name, literally: Linux-${uname -m} (CMAKE_SYSTEM_PROCESSOR is uname -m for native builds;
    # cross toolchain files set it). HostSupport also searches the compiled architecture's name.
    set(ARCHDIR "Linux-${CMAKE_SYSTEM_PROCESSOR}")
  elseif(_OFX_CPU MATCHES "^armv[2-6]" AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCHDIR "FreeBSD-armv6")
  elseif(_OFX_CPU MATCHES "^(aarch64|arm64|armv8l|arm.*)$")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(ARCHDIR "FreeBSD-aarch64")
    else()
      set(ARCHDIR "FreeBSD-armv7")
    endif()
  elseif(_OFX_OS STREQUAL "FreeBSD" AND _OFX_CPU MATCHES "^riscv")
    set(ARCHDIR "FreeBSD-riscv64")
  elseif(_OFX_OS STREQUAL "FreeBSD" AND _OFX_CPU MATCHES "^powerpc")
    # uname -p: powerpc64, powerpc64le (uname -m only says powerpc)
    if(NOT DEFINED CMAKE_CXX_BYTE_ORDER) # CMake < 3.20
      include(TestBigEndian)
      test_big_endian(_OFX_BIG_ENDIAN)
      if(NOT _OFX_BIG_ENDIAN)
        set(CMAKE_CXX_BYTE_ORDER "LITTLE_ENDIAN")
      endif()
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_CXX_BYTE_ORDER STREQUAL "LITTLE_ENDIAN")
      set(ARCHDIR "FreeBSD-powerpc64le")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(ARCHDIR "FreeBSD-powerpc64")
    else()
      set(ARCHDIR "FreeBSD-powerpc")
    endif()
  else()
    set(ARCHDIR "${_OFX_OS}-${CMAKE_SYSTEM_PROCESSOR}") # Linux: uname -m (riscv64, ppc64le, s390x, ...)
  endif()
else()
  set(ARCHDIR "unknown-arch")
endif()
set(OFX_ARCH "${ARCHDIR}")

SET(OFX_ARCH_NAME ${OFX_ARCH} CACHE STRING "OpenFX target OS and architecture")
IF("${OFX_ARCH_NAME}" STREQUAL "unknown-arch")
  MESSAGE(SEND_ERROR "OFX_ARCH_NAME was not detected. Install targets will be unavailable. See http://openeffects.org/documentation/reference/ch02s02.html to pick an arch name.")
ELSE()
  INSTALL(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
    LIBRARY DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/${OFX_ARCH_NAME}
  )
  INSTALL(FILES ${RESOURCES}
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources
  )
  INSTALL(FILES Info.plist
    DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents
  )
  if(${BUNDLE_FONTS_CONF})
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/fonts.conf
        DESTINATION ${PROJECT_NAME}.ofx.bundle/Contents/Resources/fonts
    )
  endif()
ENDIF()
