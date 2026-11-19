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
CMAKE_MINIMUM_REQUIRED(VERSION 3.16) # as Natron and openfx-io; CMake 3.16 has OpenMP imported targets
# macOS: the bundles target macOS 14.0 (VFX Reference Platform CY2027), as Natron's macOS
# plug-ins; without it, clang targets the SDK's macOS. CMake reads the deployment target at the
# first project(), so it is set here, unless CMAKE_OSX_DEPLOYMENT_TARGET, the
# MACOSX_DEPLOYMENT_TARGET environment variable or a toolchain file says otherwise. Not for cross
# builds to other Apple systems (e.g. CMAKE_SYSTEM_NAME=iOS). openfx's own default (openfx#160)
# only applies when openfx is the top-level project, and its Makefile.master defaults to 14.0 too.
if(CMAKE_HOST_APPLE AND NOT CMAKE_TOOLCHAIN_FILE AND (NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin") AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
  set(CMAKE_OSX_DEPLOYMENT_TARGET 14.0 CACHE STRING "Minimum macOS version")
endif()
PROJECT(Arena CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
endif()

# Release by default with single-config generators: CMake's empty build type has
# no optimization (and, like the Debug build, no NDEBUG)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  message(STATUS "CMAKE_BUILD_TYPE not set: using Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

option(OLD_POPPLER "Support poppler-lib < 0.83" OFF)
# AUTO, as Bundle/Makefile: AudioCurve if pkg-config finds sox
set(AUDIO "AUTO" CACHE STRING "Build AudioCurve (sox): AUTO (if pkg-config finds sox), ON or OFF")
set_property(CACHE AUDIO PROPERTY STRINGS AUTO ON OFF)
option(RICHTEXT "Enable RichText OFX" OFF)
# MinGW bundles for hosts other than Natron, which doesn't ship the MinGW DLLs
# (make LDFLAGS_ADD=-static)
option(MINGW_STATIC "MinGW: link the bundles statically (-static)" OFF)
# Not in the Arena bundle, as with the Makefiles: their own bundles
option(HALDCLUT "Also build HaldCLUT.ofx.bundle (requires curl), as Magick/HaldCLUT/Makefile" OFF)
option(OCL "Also build OCL.ofx.bundle, the OpenCL plug-ins, as OCL/Makefile" OFF)
option(BUNDLE_FONTS_CONF "Bundle fonts.conf" OFF)
set(MAGICK_PKG_CONFIG "Magick++" CACHE STRING "ImageMagick pkg-config name")
# ImageMagick variants, as the Makefiles' LEGACYIM=1 and IM=68|7
option(LEGACYIM "Old ImageMagick: leave out Sketch, Polaroid and Edges (make LEGACYIM=1)" OFF)
set(IM "" CACHE STRING "ImageMagick version: 68 (6.8: no Magick seed), 7 (MinGW: also link oleaut32), or empty (make IM=)")
set_property(CACHE IM PROPERTY STRINGS "" 68 7)
# make LICENSE=COMMERCIAL (Natron's NATRON_LICENSE): leave out the plug-ins that
# link GPL-only libraries, i.e. ReadPDF (poppler)
set(OFX_LICENSE "GPL" CACHE STRING "GPL, or COMMERCIAL to leave out ReadPDF (poppler is GPL)")
set_property(CACHE OFX_LICENSE PROPERTY STRINGS GPL COMMERCIAL)
if(NOT OFX_LICENSE MATCHES "^(GPL|COMMERCIAL)$")
  message(FATAL_ERROR "OFX_LICENSE must be GPL or COMMERCIAL, not \"${OFX_LICENSE}\"")
endif()

option(BUILD_TESTING "Build a plug-in load test (ctest)" OFF)

# Check that submodule have been initialized and updated
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/OpenFX/include" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/SupportExt/cmake/SupportExt.cmake"
   OR NOT EXISTS "${PROJECT_SOURCE_DIR}/OpenFX-IO/IOSupport/cmake/IOSupport.cmake")
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update --init --recursive\n")
endif()

# OpenGL: libGL on Linux, as Makefile.master (-lGL), not GLVND's libOpenGL
if(NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()

# supportext_add_sources(), from Arena's own SupportExt. Included first, before anything
# that could include OpenFX-IO's copy (OpenFX-IO/SupportExt, e.g. through IOSupport's
# CMake): the first SupportExt.cmake included is the one every call uses, and the
# SupportExt submodule is the one Arena pins and the Makefiles build.
include(SupportExt/cmake/SupportExt.cmake)
# ioSupport_add_sources(), from OpenFX-IO's IOSupport; after SupportExt.cmake, which it uses,
# so that its supportext_add_sources() calls use Arena's SupportExt
include(OpenFX-IO/IOSupport/cmake/IOSupport.cmake)

# openfx::Api and openfx::Support (with the OFX_EXTENSIONS_* and OFX_SUPPORTS_OPENGLRENDER
# definitions of Makefile.master), and cmake/OpenFX.cmake: add_ofx_plugin(),
# ofx_plugin_resources(), ofx_target_openmp(), ofx_deploy_bundle(), ARCHDIR, and the
# OFX_CXX_STANDARD, OFX_USE_OPENMP, OFX_DEPLOY_BUNDLES and PLUGIN_INSTALLDIR (default
# OFX/Plugins, relative to CMAKE_INSTALL_PREFIX) cache variables. EXCLUDE_FROM_ALL: build
# only what Arena links. With BUILD_TESTING, also openfx's hostDemo, for the render test
# (tests/hostdemo); normal builds don't build it.
if(BUILD_TESTING)
  set(BUILD_EXAMPLE_HOSTS ON)
endif()
# SupportExt's OFX 1.5 DrawSuite overlay backend (SupportExt/ofxsOverlayDraw.h), as in
# Makefile.master and Arena.xcodeproj. It must have the same value in every translation unit
# (ODR: it changes class layouts and inline functions; MSVC checks it with
# #pragma detect_mismatch), so it is set for the whole tree, before add_subdirectory(OpenFX).
add_compile_definitions(OFXS_HAVE_DRAWSUITE=1)
add_subdirectory(OpenFX EXCLUDE_FROM_ALL)



# Libraries, found by pkg-config as in Makefile.master. Like Makefile.master, link
# with "pkg-config --static --libs": e.g. MacPorts' ImageMagick +natron (Natron's
# SDK) is a static library, which needs the libraries of its delegates.
find_package(PkgConfig REQUIRED)
set(ARENA_LIBRARIES "")
macro(arena_pkg_check_modules _prefix)
  pkg_check_modules(${_prefix} REQUIRED IMPORTED_TARGET ${ARGN})
  set(_libs "")
  set(_framework FALSE)
  foreach(_f ${${_prefix}_STATIC_LDFLAGS})
    if(_framework)
      list(APPEND _libs "-Wl,-framework,${_f}") # one item: CMake would split "-framework X"
      set(_framework FALSE)
    elseif(_f STREQUAL "-framework")
      set(_framework TRUE)
    else()
      list(APPEND _libs "${_f}")
    endif()
  endforeach()
  set_target_properties(PkgConfig::${_prefix} PROPERTIES INTERFACE_LINK_LIBRARIES "${_libs}" INTERFACE_LINK_OPTIONS "")
  list(APPEND ARENA_LIBRARIES PkgConfig::${_prefix})
endmacro()
arena_pkg_check_modules(MAGICK ${MAGICK_PKG_CONFIG})
arena_pkg_check_modules(LCMS2 lcms2)
arena_pkg_check_modules(FONTCONFIG fontconfig)
arena_pkg_check_modules(PANGO pangocairo)
arena_pkg_check_modules(XML libxml-2.0)
arena_pkg_check_modules(ZIP libzip)
arena_pkg_check_modules(SVG librsvg-2.0)
arena_pkg_check_modules(GLIB glib-2.0)
arena_pkg_check_modules(CAIRO cairo)
if(OFX_LICENSE STREQUAL "GPL")
  pkg_check_modules(POPPLER_PROBE QUIET poppler-glib)
  if(NOT POPPLER_PROBE_FOUND)
    message(FATAL_ERROR "poppler-glib not found (needed by ReadPDF): install poppler-glib or use -DOFX_LICENSE=COMMERCIAL")
  endif()
  arena_pkg_check_modules(POPPLER poppler-glib)
endif()
arena_pkg_check_modules(REVENGE librevenge-0.0 librevenge-generators-0.0 librevenge-stream-0.0)
arena_pkg_check_modules(CDR libcdr-0.1)
string(TOUPPER "${AUDIO}" _audio)
if(NOT _audio MATCHES "^(AUTO|ON|OFF)$")
  message(FATAL_ERROR "AUDIO must be AUTO, ON or OFF, not \"${AUDIO}\"")
endif()
if(_audio STREQUAL "AUTO")
  pkg_check_modules(SOX_PROBE QUIET sox)
  if(SOX_PROBE_FOUND)
    set(BUILD_AUDIO ON)
  else()
    set(BUILD_AUDIO OFF)
  endif()
elseif(_audio STREQUAL "ON")
  set(BUILD_AUDIO ON)
else()
  set(BUILD_AUDIO OFF)
endif()
message(STATUS "AudioCurve (AUDIO=${AUDIO}): ${BUILD_AUDIO}")
if(BUILD_AUDIO)
  arena_pkg_check_modules(SOX sox)
endif()
if(HALDCLUT)
  # HaldCLUT uses libCurl to download ~100MiB of CLUT profiles
  arena_pkg_check_modules(CURL libcurl)
  list(REMOVE_ITEM ARENA_LIBRARIES PkgConfig::CURL) # HaldCLUT's own bundle
endif()

# OpenColorIO, for GenericReader's colorspace parameters, as Makefile.io (which,
# unlike Makefile.master, uses the shared link line)
pkg_check_modules(OCIO REQUIRED IMPORTED_TARGET OpenColorIO)
list(APPEND ARENA_LIBRARIES PkgConfig::OCIO)

# ImageMagick's .pc passes on the -fopenmp it was built with, which Apple clang
# rejects: drop it, and let ofx_target_openmp() add OpenMP (OFX_USE_OPENMP). Keep
# its OpenMP runtime (-lomp, -lgomp) in the link, which a static ImageMagick needs:
# ofx_target_openmp() puts libomp's directory on the link path (or see after
# ofx_target_openmp(Arena) below).
foreach(_prop INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_LIBRARIES)
  get_target_property(_flags PkgConfig::MAGICK ${_prop})
  if(_flags)
    list(REMOVE_ITEM _flags -fopenmp)
    set_property(TARGET PkgConfig::MAGICK PROPERTY ${_prop} "${_flags}")
  endif()
endforeach()
if(MINGW)
  # as Makefile.master's MINGW=1 (and IM=7)
  set_property(TARGET PkgConfig::MAGICK APPEND PROPERTY INTERFACE_LINK_LIBRARIES gdi32 freetype expat z ws2_32)
  if(IM STREQUAL "7")
    set_property(TARGET PkgConfig::MAGICK APPEND PROPERTY INTERFACE_LINK_LIBRARIES oleaut32)
  endif()
endif()
if(NOT DEFINED OFX_LIBOMP_ROOT)
  set(OFX_LIBOMP_ROOT "${MAGICK_PREFIX}") # ofx_target_openmp(): libomp next to ImageMagick, else MacPorts
endif()

# The plug-ins and resources of Bundle/Makefile, in the same order, which is the
# order of the plug-ins in the bundle (except ReadPDF, built separately, see below)
set(SOURCES
    lodepng/lodepng.cpp
    Text/RichText.cpp
    Text/TextOFX.cpp
    Extra/ReadCDR.cpp
    Extra/ReadSVG.cpp
    Extra/ReadKrita.cpp
    Extra/OpenRaster.cpp)
set(RESOURCES
    Text/net.fxarena.openfx.Text.png
    Text/net.fxarena.openfx.Text.svg
    Extra/fr.inria.openfx.ReadCDR.png
    Extra/fr.inria.openfx.ReadCDR.svg
    Extra/net.fxarena.openfx.ReadSVG.png
    Extra/net.fxarena.openfx.ReadSVG.svg
    Extra/fr.inria.openfx.ReadKrita.png
    Extra/fr.inria.openfx.ReadKrita.svg
    Extra/fr.inria.openfx.OpenRaster.png
    Extra/fr.inria.openfx.OpenRaster.svg)
if(BUILD_AUDIO)
    list(APPEND SOURCES Audio/AudioCurve.cpp)
    list(APPEND RESOURCES Audio/net.fxarena.openfx.AudioCurve.png)
endif()
if(RICHTEXT)
    list(APPEND SOURCES Text/RichTextOFX.cpp)
    list(APPEND RESOURCES Text/net.fxarena.openfx.RichText.png Text/net.fxarena.openfx.RichText.svg)
endif()
if(OFX_LICENSE STREQUAL "GPL")
    # ReadPDF.cpp is built in the ArenaReadPDF object library (see below)
    list(APPEND RESOURCES Extra/fr.inria.openfx.ReadPDF.png Extra/fr.inria.openfx.ReadPDF.svg)
endif()
if(NOT LEGACYIM)
    list(APPEND SOURCES
        Magick/Sketch.cpp
        Magick/Polaroid.cpp
        Magick/Edges.cpp)
    list(APPEND RESOURCES
        Magick/net.fxarena.openfx.Sketch.png
        Magick/net.fxarena.openfx.Sketch.svg
        Magick/net.fxarena.openfx.Polaroid.png
        Magick/net.fxarena.openfx.Polaroid.svg
        Magick/net.fxarena.openfx.Edges.png
        Magick/net.fxarena.openfx.Edges.svg)
endif()
list(APPEND SOURCES
    Magick/Texture.cpp
    Magick/Tile.cpp
    Magick/Reflection.cpp
    Magick/Arc.cpp
    Magick/Polar.cpp
    Magick/Roll/Roll.cpp
    Magick/Wave/Wave.cpp
    Magick/Swirl/Swirl.cpp
    Magick/Implode.cpp
    Magick/Charcoal.cpp
    Magick/Oilpaint.cpp
    Magick/ReadPSD.cpp
    Magick/Modulate.cpp
    Magick/ReadMisc.cpp
    Magick/Text.cpp
    Magick/Morphology/Morphology.cpp
    Magick/MagickPlugin.cpp)
# Magick/Text.cpp has the identifier of Text/TextOFX.cpp: only TextOFX's icons are
# in the bundle
list(APPEND RESOURCES
    Magick/Extra.png
    Magick/Extra.svg
    Magick/net.fxarena.openfx.Tile.png
    Magick/net.fxarena.openfx.Tile.svg
    Magick/net.fxarena.openfx.Reflection.png
    Magick/net.fxarena.openfx.Reflection.svg
    Magick/net.fxarena.openfx.Arc.png
    Magick/net.fxarena.openfx.Arc.svg
    Magick/net.fxarena.openfx.Polar.png
    Magick/net.fxarena.openfx.Polar.svg
    Magick/Roll/net.fxarena.openfx.Roll.png
    Magick/Roll/net.fxarena.openfx.Roll.svg
    Magick/Wave/net.fxarena.openfx.Wave.png
    Magick/Wave/net.fxarena.openfx.Wave.svg
    Magick/Swirl/net.fxarena.openfx.Swirl.png
    Magick/Swirl/net.fxarena.openfx.Swirl.svg
    Magick/net.fxarena.openfx.Implode.png
    Magick/net.fxarena.openfx.Implode.svg
    Magick/net.fxarena.openfx.Charcoal.png
    Magick/net.fxarena.openfx.Charcoal.svg
    Magick/net.fxarena.openfx.Oilpaint.png
    Magick/net.fxarena.openfx.Oilpaint.svg
    Magick/net.fxarena.openfx.ReadPSD.png
    Magick/net.fxarena.openfx.ReadPSD.svg
    Magick/net.fxarena.openfx.Modulate.png
    Magick/net.fxarena.openfx.Modulate.svg
    Magick/fr.inria.openfx.ReadMisc.png
    Magick/fr.inria.openfx.ReadMisc.svg
    Magick/net.fxarena.openfx.Texture.png
    Magick/net.fxarena.openfx.Texture.svg
    Magick/Morphology/net.fxarena.openfx.Morphology.png
    Magick/Morphology/net.fxarena.openfx.Morphology.svg)

# No RPATH into OpenColorIO's libdir, as Makefile.io: a Linux bundle's RUNPATH only has
# $ORIGIN-relative entries (openfx's linuxCheckBundle.sh); the installers bundle OpenColorIO
add_ofx_plugin(Arena INFO_PLIST Bundle/Info.plist SOURCES ${SOURCES})
supportext_add_sources(Arena COMPONENTS TEXT TRANSFORM3X3 TRANSFORM_INTERACT MULTIPLANE)
# GenericReader, with GenericOCIO and SequenceParsing. OpenColorIO is PkgConfig::OCIO, not
# OpenColorIO::OpenColorIO: the helper adds neither GenericOCIO's OpenGL code nor
# OFX_IO_USING_OCIO, which Arena sets itself (below), as Makefile.io
ioSupport_add_sources(Arena COMPONENTS GenericReader)
target_include_directories(Arena PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick"
    "${CMAKE_CURRENT_SOURCE_DIR}/Extra"
    "${CMAKE_CURRENT_SOURCE_DIR}/Audio"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bundle"
    "${CMAKE_CURRENT_SOURCE_DIR}/Text"
    "${CMAKE_CURRENT_SOURCE_DIR}/lodepng")
target_link_libraries(Arena PRIVATE openfx::Support ${ARENA_LIBRARIES})
ofx_target_openmp(Arena)
# ImageMagick's -lomp needs the directory of libomp: with Apple clang, ofx_target_openmp() puts
# the libomp it links on the link path. Otherwise (OFX_USE_OPENMP=OFF, no libomp found, or
# another compiler, e.g. MacPorts clang, which links libomp by full path), as
# Makefile.master's LIBOMP_PREFIX: next to ImageMagick, else MacPorts' (macOS).
if(NOT TARGET OpenMP::OpenMP_CXX OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(EXISTS "${MAGICK_PREFIX}/lib/libomp")
    set(_libomp_dir "${MAGICK_PREFIX}/lib/libomp")
  elseif(APPLE AND EXISTS "/opt/local/lib/libomp")
    set(_libomp_dir "/opt/local/lib/libomp")
  endif()
  if(_libomp_dir)
    # an interface property, read at generation: also applies to the targets linked
    # with PkgConfig::MAGICK before this point
    set_property(TARGET PkgConfig::MAGICK APPEND PROPERTY INTERFACE_LINK_DIRECTORIES "${_libomp_dir}")
  endif()
endif()
target_compile_definitions(Arena PRIVATE OFX_IO_USING_OCIO)
if(LEGACYIM)
    target_compile_definitions(Arena PRIVATE LEGACYIM NOMAGICKSEED)
elseif(IM STREQUAL "68")
    target_compile_definitions(Arena PRIVATE NOMAGICKSEED)
endif()
if(OLD_POPPLER)
    target_compile_definitions(Arena PRIVATE LEGACY_POPPLER)
endif()

if(OFX_LICENSE STREQUAL "GPL")
    # ReadPDF.cpp includes poppler's C++ headers (GlobalParams.h), which need
    # C++17 since poppler 21.12 (#23) and C++20 (std::span) in recent versions.
    # Only that file is built with this standard, with GNU extensions (gnu++2a,
    # as Makefile.master: MinGW GCC 16 headers need them in C++20 mode), in an
    # object library that is otherwise built like the other Arena sources.
    set(POPPLER_CXX_STANDARD 20 CACHE STRING "C++ standard of ReadPDF.cpp (Makefile.master: POPPLER_CXXSTD)")
    set(_std ${POPPLER_CXX_STANDARD})
    if(OFX_CXX_STANDARD GREATER _std)
        set(_std ${OFX_CXX_STANDARD})
    endif()
    add_library(ArenaReadPDF OBJECT Extra/ReadPDF.cpp)
    set_target_properties(ArenaReadPDF PROPERTIES
        CXX_STANDARD ${_std} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS ON
        POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    target_include_directories(ArenaReadPDF PRIVATE $<TARGET_PROPERTY:Arena,INCLUDE_DIRECTORIES>)
    target_compile_definitions(ArenaReadPDF PRIVATE $<TARGET_PROPERTY:Arena,COMPILE_DEFINITIONS>)
    target_compile_options(ArenaReadPDF PRIVATE $<TARGET_PROPERTY:Arena,COMPILE_OPTIONS>)
    target_link_libraries(ArenaReadPDF PRIVATE openfx::Support ${ARENA_LIBRARIES})
    ofx_target_openmp(ArenaReadPDF)
    target_link_libraries(Arena PRIVATE ArenaReadPDF)
endif()

ofx_plugin_resources(Arena ${RESOURCES})

# fonts.conf
if(BUNDLE_FONTS_CONF)
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
        "${CMAKE_CURRENT_SOURCE_DIR}/Text/fonts.conf.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fonts.conf"
        @ONLY)
    ofx_plugin_resources(Arena DESTINATION fonts "${CMAKE_CURRENT_BINARY_DIR}/fonts.conf")
endif()

ofx_deploy_bundle(Arena)

# HaldCLUT.ofx.bundle, as Magick/HaldCLUT/Makefile (with Magick/Makefile.Magick)
if(HALDCLUT)
    add_ofx_plugin(HaldCLUT INFO_PLIST Magick/HaldCLUT/Info.plist
        SOURCES Magick/HaldCLUT/HaldCLUT.cpp Magick/MagickPlugin.cpp)
    supportext_add_sources(HaldCLUT)
    target_include_directories(HaldCLUT PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Magick")
    target_link_libraries(HaldCLUT PRIVATE openfx::Support PkgConfig::MAGICK PkgConfig::CURL PkgConfig::XML)
    ofx_target_openmp(HaldCLUT)
    if(LEGACYIM)
        target_compile_definitions(HaldCLUT PRIVATE LEGACYIM NOMAGICKSEED)
    elseif(IM STREQUAL "68")
        target_compile_definitions(HaldCLUT PRIVATE NOMAGICKSEED)
    endif()
    ofx_plugin_resources(HaldCLUT
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.png
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.svg
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.xml
        Magick/Extra.png
        Magick/Extra.svg)
    ofx_deploy_bundle(HaldCLUT)
endif()

# OCL.ofx.bundle, as OCL/Makefile: the system OpenCL (on macOS, the framework), or the
# headers and libraries in OCL/OpenCL
if(OCL)
    if(NOT APPLE)
        if(NOT OpenCL_INCLUDE_DIR)
            set(OpenCL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/OCL/OpenCL" CACHE PATH "OpenCL headers")
        endif()
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_ocl_lib "${CMAKE_CURRENT_SOURCE_DIR}/OCL/OpenCL/lib64")
        else()
            set(_ocl_lib "${CMAKE_CURRENT_SOURCE_DIR}/OCL/OpenCL/lib32")
        endif()
        find_library(OpenCL_LIBRARY NAMES OpenCL HINTS "${_ocl_lib}")
    endif()
    find_package(OpenCL REQUIRED)
    set(_ocl_plugins Bulge Cartoon Duotone Edge Ripple Sharpen Twirl)
    set(_ocl_sources OCL/OCLPlugin.cpp OCL/ofxsTransformInteractCustom.cpp)
    set(_ocl_resources OCL/OpenCL.png)
    foreach(_p ${_ocl_plugins})
        list(APPEND _ocl_sources OCL/${_p}/${_p}.cpp)
        list(APPEND _ocl_resources
            OCL/${_p}/net.fxarena.opencl.${_p}.png
            OCL/${_p}/net.fxarena.opencl.${_p}.svg
            OCL/${_p}/net.fxarena.opencl.${_p}.cl)
    endforeach()
    add_ofx_plugin(OCL INFO_PLIST OCL/Info.plist SOURCES ${_ocl_sources})
    supportext_add_sources(OCL COMPONENTS TRANSFORM3X3)
    target_include_directories(OCL PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/OCL")
    target_link_libraries(OCL PRIVATE openfx::Support OpenCL::OpenCL)
    # ofxsTransformInteractCustom.cpp draws with OpenGL
    find_package(OpenGL REQUIRED)
    target_link_libraries(OCL PRIVATE OpenGL::GL)
    ofx_plugin_resources(OCL ${_ocl_resources})
    ofx_deploy_bundle(OCL)
endif()

foreach(_t Arena HaldCLUT OCL)
    if(TARGET ${_t})
        # as Makefile.master's CONFIG=debug
        target_compile_options(${_t} PRIVATE "$<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wall;-Wextra>")
        if(MINGW AND MINGW_STATIC)
            target_link_options(${_t} PRIVATE -static)
        endif()
        if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            # as Makefile.master's FREEBSD=1
            target_include_directories(${_t} PRIVATE /usr/local/include)
            target_link_directories(${_t} PRIVATE /usr/local/lib)
        endif()
    endif()
endforeach()

# ctest: load each bundle with the plug-in load test of the CI (OpenFX-IO's
# verify_plugin_loads, with RTLD_NOW so that a missing symbol fails), which prints
# Num_plugins
if(BUILD_TESTING)
    enable_testing()
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/OpenFX/HostSupport/src/ofxhBinary.cpp" _binary)
    string(REPLACE "RTLD_LAZY" "RTLD_NOW" _binary "${_binary}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ofxhBinaryStrict.cpp" "${_binary}")
    add_executable(verify_plugin_loads
        OpenFX-IO/.github/workflows/verify_plugin_loads.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/ofxhBinaryStrict.cpp")
    if(WIN32)
        target_sources(verify_plugin_loads PRIVATE OpenFX/HostSupport/src/ofxhUtilities.cpp)
    endif()
    target_include_directories(verify_plugin_loads PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/OpenFX/HostSupport/include" "${CMAKE_CURRENT_SOURCE_DIR}/OpenFX/include")
    target_link_libraries(verify_plugin_loads PRIVATE ${CMAKE_DL_LIBS})
    # the number of plug-ins each bundle must have
    set(_n_Arena 41) # with AudioCurve and ReadPDF, without RichText: 43, as the CI checks
    if(BUILD_AUDIO)
        math(EXPR _n_Arena "${_n_Arena} + 1")
    endif()
    if(OFX_LICENSE STREQUAL "GPL")
        math(EXPR _n_Arena "${_n_Arena} + 1")
    endif()
    if(LEGACYIM)
        # Sketch, Polaroid and Edges, two major versions each
        math(EXPR _n_Arena "${_n_Arena} - 6")
    endif()
    if(RICHTEXT)
        math(EXPR _n_Arena "${_n_Arena} + 1")
    endif()
    set(_n_HaldCLUT 2)
    set(_n_OCL 9)
    # fail on a non-zero exit status, and unless the count is printed
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/verify_plugin_loads.cmake" [=[
execute_process(COMMAND "${VERIFY}" "${PLUGIN}" RESULT_VARIABLE _r OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
message("${_out}${_err}")
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "verify_plugin_loads exited with ${_r}")
endif()
if(NOT _out MATCHES "Num_plugins: ${COUNT}\r?\n")
  message(FATAL_ERROR "expected Num_plugins: ${COUNT}")
endif()
]=])
    foreach(_t Arena HaldCLUT OCL)
        if(TARGET ${_t})
            add_test(NAME load_${_t} COMMAND "${CMAKE_COMMAND}"
                -DVERIFY=$<TARGET_FILE:verify_plugin_loads> -DPLUGIN=$<TARGET_FILE:${_t}> -DCOUNT=${_n_${_t}}
                -P "${CMAKE_CURRENT_BINARY_DIR}/verify_plugin_loads.cmake")
        endif()
    endforeach()

    # render test: every plug-in rendered once with openfx's hostDemo (tests/hostdemo), the
    # readers on samples written by make_samples (ImageMagick, libzip) and sample.svg
    if(TARGET hostDemo)
        set(_samples "${CMAKE_CURRENT_BINARY_DIR}/hostdemo-samples")
        add_executable(make_samples tests/hostdemo/make_samples.cpp)
        target_link_libraries(make_samples PRIVATE PkgConfig::MAGICK PkgConfig::ZIP)
        ofx_target_openmp(make_samples)
        set(_sample_files sample.png sample.psd sample.bmp sample.ora sample.kra sample.pdf sample.svg sample_hald.png
            sample_xy.png sample_xy.ora sample_xy.kra sample_xy.pdf)
        list(TRANSFORM _sample_files PREPEND "${_samples}/")
        add_custom_command(OUTPUT ${_sample_files}
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${_samples}"
            COMMAND make_samples "${_samples}"
            COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/sample.svg" "${_samples}"
            DEPENDS make_samples tests/hostdemo/sample.svg
            VERBATIM)
        # hostDemo is in openfx's EXCLUDE_FROM_ALL directory: build it with the tests
        add_custom_target(hostdemo_test ALL DEPENDS ${_sample_files} hostDemo)
        foreach(_t Arena HaldCLUT OCL)
            if(TARGET ${_t})
                string(TOLOWER "${_t}" _list)
                add_test(NAME render_${_t} COMMAND "${CMAKE_COMMAND}"
                    -DHOSTDEMO=$<TARGET_FILE:hostDemo> -DVERIFY=$<TARGET_FILE:verify_plugin_loads>
                    -DBINARY=$<TARGET_FILE:${_t}> -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/${_list}.txt
                    -DSAMPLES=${_samples} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-${_t}
                    -DCACHE=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-cache
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/run.cmake")
            endif()
        endforeach()
        # the same, with hostDemo as an OFX 1.4 host (--ofx14): every plug-in must also work
        # in a host that has neither OFX 1.5 nor the extensions (HaldCLUT and OCL, which the
        # CI doesn't build, only get the default pass)
        if(TARGET Arena)
            add_test(NAME render_Arena_ofx14 COMMAND "${CMAKE_COMMAND}"
                -DHOSTDEMO=$<TARGET_FILE:hostDemo> -DVERIFY=$<TARGET_FILE:verify_plugin_loads>
                -DBINARY=$<TARGET_FILE:Arena> -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/arena.txt
                -DSAMPLES=${_samples} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-Arena-ofx14
                -DCACHE=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-cache -DOFX14=ON
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/run.cmake")
            # render_Arena runs hostDemo in its default mode, which unloads the binary after the
            # scan and loads it again at the first use (two load cycles per run, openfx #174);
            # this pass keeps it loaded (OFX_HOSTDEMO_KEEP_LOADED=1), as Natron 2.6 will
            add_test(NAME render_Arena_keep_loaded COMMAND "${CMAKE_COMMAND}"
                -DHOSTDEMO=$<TARGET_FILE:hostDemo> -DVERIFY=$<TARGET_FILE:verify_plugin_loads>
                -DBINARY=$<TARGET_FILE:Arena> -DLIST=${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/arena.txt
                -DSAMPLES=${_samples} -DWORK=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-Arena-keep-loaded
                -DCACHE=${CMAKE_CURRENT_BINARY_DIR}/hostdemo-cache -DKEEP_LOADED=ON
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/run.cmake")
        endif()

        # ReadSVG's output size at several DPIs, in physical and user units, for both major
        # versions (tests/readsvg); string(JSON) needs CMake 3.19
        if(TARGET Arena AND NOT CMAKE_VERSION VERSION_LESS 3.19)
            set(_exact OFF)
            if(NOT SVG_VERSION VERSION_LESS 2.52)
                set(_exact ON) # rsvg_handle_get_intrinsic_size_in_pixels()
            endif()
            add_test(NAME readsvg_dpi COMMAND "${CMAKE_COMMAND}"
                -DHOSTDEMO=$<TARGET_FILE:hostDemo> -DPLUGIN_PATH=${OFX_PLUGIN_BUILD_DIR}
                -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/tests/readsvg
                -DOCIO=${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/config.ocio
                -DWORK=${CMAKE_CURRENT_BINARY_DIR}/readsvg-dpi -DEXACT=${_exact}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/readsvg/dpi.cmake")
        endif()

        # the readers with render windows smaller than the image (#105, tests/readerwindow):
        # reader_window is a small host built from hostDemo's host classes, since hostDemo
        # renders the whole RoD of a plug-in without tiles, whatever --window
        if(TARGET Arena)
            set(_hd "${CMAKE_CURRENT_SOURCE_DIR}/OpenFX/HostSupport/examples")
            add_executable(reader_window tests/readerwindow/reader_window.cpp
                "${_hd}/hostDemoClipInstance.cpp" "${_hd}/hostDemoEffectInstance.cpp"
                "${_hd}/hostDemoHostDescriptor.cpp" "${_hd}/hostDemoParamInstance.cpp")
            target_include_directories(reader_window PRIVATE "${_hd}")
            target_link_libraries(reader_window PRIVATE OfxHost)
            add_test(NAME reader_window COMMAND "${CMAKE_COMMAND}"
                -DREADER_WINDOW=$<TARGET_FILE:reader_window> -DPLUGIN_PATH=${OFX_PLUGIN_BUILD_DIR}
                -DSAMPLES=${_samples} -DOCIO=${CMAKE_CURRENT_SOURCE_DIR}/tests/hostdemo/config.ocio
                -DWORK=${CMAKE_CURRENT_BINARY_DIR}/reader-window
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/readerwindow/run.cmake")
        endif()

        # ReadPSD's layer menu (tests/readpsd, also "make -C tests check"): needs Python 3 with
        # NumPy, and ImageMagick's magick or convert for the layered PSD (mklayers.py finds it)
        # (python3 from the PATH, as "make -C tests check": FindPython3 may pick another one)
        find_program(ARENA_PYTHON3 python3)
        set(_numpy FALSE)
        if(ARENA_PYTHON3)
            # Python 3.6 or later (Rocky 8's), with NumPy
            execute_process(COMMAND "${ARENA_PYTHON3}" -c "import sys; assert sys.version_info >= (3, 6); import numpy"
                RESULT_VARIABLE _r OUTPUT_QUIET ERROR_QUIET)
            if(_r EQUAL 0)
                set(_numpy TRUE)
            endif()
        endif()
        # magick or convert next to Magick++, else magick, else a convert that is ImageMagick's
        # (Windows has System32\convert.exe)
        find_program(ARENA_MAGICK_TOOL NAMES magick convert HINTS "${MAGICK_PREFIX}/bin" NO_DEFAULT_PATH)
        find_program(ARENA_MAGICK_TOOL NAMES magick)
        if(NOT ARENA_MAGICK_TOOL)
            find_program(_arena_convert convert)
            if(_arena_convert)
                execute_process(COMMAND "${_arena_convert}" -version OUTPUT_VARIABLE _v ERROR_QUIET
                    RESULT_VARIABLE _r TIMEOUT 30)
                if(_r EQUAL 0 AND _v MATCHES "ImageMagick")
                    set(ARENA_MAGICK_TOOL "${_arena_convert}" CACHE FILEPATH "ImageMagick's magick or convert" FORCE)
                endif()
            endif()
        endif()
        if(_numpy AND ARENA_MAGICK_TOOL)
            add_test(NAME readpsd_layers COMMAND "${ARENA_PYTHON3}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/readpsd/test_layers.py"
                --hostdemo $<TARGET_FILE:hostDemo> "${OFX_PLUGIN_BUILD_DIR}")
            set_tests_properties(readpsd_layers PROPERTIES ENVIRONMENT "MAGICK=${ARENA_MAGICK_TOOL}")
        else()
            message(STATUS "BUILD_TESTING: no ReadPSD layer test (tests/readpsd needs Python 3.6 or later with NumPy, and ImageMagick's magick or convert)")
        endif()
    else()
        message(STATUS "BUILD_TESTING: no hostDemo (openfx's HostSupport examples), no render test")
    endif()
endif()
