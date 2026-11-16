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
PROJECT(Arena CXX)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
endif()

option(OLD_POPPLER "Support poppler-lib < 0.83" OFF)
option(AUDIO "Enable AudioCurve OFX" OFF)
option(RICHTEXT "Enable RichText OFX" OFF)
option(HALDCLUT "Enable HaldCLUT OFX (requires curl)" OFF)
option(BUNDLE_FONTS_CONF "Bundle fonts.conf" OFF)
option(READMISC_LEGACY "Enable legacy file formats in ReadMisc" OFF)
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

# Check that submodule have been initialized and updated
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/OpenFX/include" OR NOT EXISTS "${PROJECT_SOURCE_DIR}/SupportExt/ofxsMacros.h"
   OR NOT EXISTS "${PROJECT_SOURCE_DIR}/OpenFX-IO/IOSupport/SequenceParsing/SequenceParsing.cpp")
  message(FATAL_ERROR
    "\n submodule(s) are missing, please update your repository:\n"
    "  > git submodule update --init --recursive\n")
endif()

# openfx::Api and openfx::Support (with the OFX_EXTENSIONS_* and
# OFX_SUPPORTS_OPENGLRENDER definitions of Makefile.master). EXCLUDE_FROM_ALL:
# build only what Arena links, and install none of openfx.
add_subdirectory(OpenFX EXCLUDE_FROM_ALL)
# ARCHDIR, OFX_PLUGIN_BUILD_DIR and add_ofx_plugin() (redefined below until
# OpenFX.cmake has the arguments we need)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/OpenFX/cmake")
include(OpenFX)

########################################################################
# TODO: replace with OpenFX.cmake, SupportExt and IOSupport when available.
#
# Stand-ins, with the names and arguments that openfx's cmake/OpenFX.cmake,
# SupportExt and OpenFX-IO/IOSupport will provide:
#   add_ofx_plugin(<target> [DIR d] [BUNDLE_NAME n] [INFO_PLIST file]
#                  [IDENTIFIER id] [VERSION v] [SOURCES ...])
#   ofx_plugin_resources(<target> [DESTINATION subdir] files...)
#   ofx_target_openmp(<target> [REQUIRED])        (OFX_USE_OPENMP)
#   ofx_deploy_bundle(<target>)                    (OFX_DEPLOY_BUNDLES)
#   supportext_add_sources(<target> [ALL] [OPENGL] [OSMESA] [EGL] [COMPONENTS ...])
#   ioSupport_add_sources(<target>)
# plus the MinGW/MSVC definitions as usage requirements of openfx::Api, and a
# static, PIC, hidden openfx::Support. Nothing Arena-specific goes in this
# block: when the submodules provide these, delete it; the calls stay.
########################################################################
set(OFX_CXX_STANDARD 17 CACHE STRING "C++ standard of the plug-ins (17: Natron 2.6 Qt5; 20: Qt6 hosts)")
option(OFX_USE_OPENMP "Build the plug-ins that ask for it with OpenMP" ON)
option(OFX_DEPLOY_BUNDLES "Copy the non-system libraries into the installed bundles (macOS: osxDeploy.sh, in Release and RelWithDebInfo)" OFF)

# Where the bundles are installed: OFX/Plugins, relative to CMAKE_INSTALL_PREFIX, unless
# given (OpenFX.cmake at 366b9709 defaults to its examples' folder)
if("$CACHE{PLUGIN_INSTALLDIR}" STREQUAL "")
  set(PLUGIN_INSTALLDIR "OFX/Plugins")
else()
  set(PLUGIN_INSTALLDIR "$CACHE{PLUGIN_INSTALLDIR}")
endif()

set(_OFX_SUPPORT_INCLUDE_DIR "${_OFX_CMAKE_MODULE_DIR}/../Support/include") # osxSymbols, linuxSymbols, osxDeploy.sh

# Definitions that Support/Plugins/Makefile.master and openfx's CMakeLists.txt
# use for MinGW and MSVC, as usage requirements of openfx::Api
get_target_property(_ofx_api openfx::Api ALIASED_TARGET)
if(MINGW)
  # math constants (M_PI) in every dialect, C99 printf
  target_compile_definitions(${_ofx_api} INTERFACE _USE_MATH_DEFINES __USE_MINGW_ANSI_STDIO=1)
elseif(MSVC)
  target_compile_definitions(${_ofx_api} INTERFACE _USE_MATH_DEFINES NOMINMAX _HAS_AUTO_PTR_ETC)
endif()
# openfx::Support: static, PIC, hidden, same standard and architectures as the plug-ins
# (openfx's CMakeLists.txt builds universal macOS binaries by default)
get_target_property(_ofx_support openfx::Support ALIASED_TARGET)
set_target_properties(${_ofx_support} PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  CXX_STANDARD ${OFX_CXX_STANDARD}
  OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
# ofxsProcessing.H and friends, which Makefile.master puts in the include path
target_include_directories(${_ofx_support} INTERFACE "$<BUILD_INTERFACE:${_OFX_CMAKE_MODULE_DIR}/../Support/Plugins/include>")

# add_ofx_plugin(<target> [DIR d] [BUNDLE_NAME n] [INFO_PLIST file] [IDENTIFIER id]
#                [VERSION v] [SOURCES ...])
# A plug-in module <BUNDLE_NAME>.ofx (BUNDLE_NAME defaults to <target>), assembled in
# ${OFX_PLUGIN_BUILD_DIR}/<BUNDLE_NAME>.ofx.bundle and installed into
# ${PLUGIN_INSTALLDIR}. INFO_PLIST is configured into Contents/Info.plist; without
# it, openfx's Info.plist.in is, with IDENTIFIER and VERSION. As Makefile.master:
# hidden visibility and the osxSymbols/linuxSymbols export lists, the rpaths of the
# bundled libraries, DEBUG or NDEBUG, big objects on Windows, ${OFX_CXX_STANDARD}.
# (DIR is accepted for compatibility with OpenFX.cmake and unused.)
function(add_ofx_plugin TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 _ofx "" "DIR;BUNDLE_NAME;INFO_PLIST;IDENTIFIER;VERSION" "SOURCES")
  if(NOT _ofx_BUNDLE_NAME)
    set(_ofx_BUNDLE_NAME "${TARGET}")
  endif()
  # MODULE everywhere: -bundle on macOS, no SONAME or import library elsewhere, as Makefile.master
  add_library(${TARGET} MODULE ${_ofx_SOURCES})
  set_target_properties(${TARGET} PROPERTIES
    PREFIX "" SUFFIX ".ofx" OUTPUT_NAME "${_ofx_BUNDLE_NAME}"
    CXX_STANDARD ${OFX_CXX_STANDARD} CXX_STANDARD_REQUIRED ON
    C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
  target_link_libraries(${TARGET} PRIVATE openfx::Support)
  target_compile_definitions(${TARGET} PRIVATE $<IF:$<CONFIG:Debug>,DEBUG,NDEBUG>)
  target_compile_options(${TARGET} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
  if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${TARGET} PRIVATE -Wa,-mbig-obj)
  endif()

  # export only OfxGetPlugin and OfxGetNumberOfPlugins
  set(_symbols "")
  if(APPLE)
    set(_symbols "${_OFX_SUPPORT_INCLUDE_DIR}/osxSymbols")
    target_link_options(${TARGET} PRIVATE "LINKER:-exported_symbols_list,${_symbols}")
  elseif(MINGW OR UNIX)
    set(_symbols "${_OFX_SUPPORT_INCLUDE_DIR}/linuxSymbols")
    target_link_options(${TARGET} PRIVATE "LINKER:--version-script=${_symbols}")
  endif()
  if(_symbols)
    set_property(TARGET ${TARGET} APPEND PROPERTY LINK_DEPENDS "${_symbols}")
  endif()
  # libraries bundled by osxDeploy.sh (macOS) or by the Natron installer (Linux)
  if(APPLE)
    set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "@loader_path/../Frameworks;@loader_path/../Libraries")
  elseif(UNIX)
    set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN/../../Libraries")
  endif()

  # the bundle, in the build tree and installed
  set(_bundle "${OFX_PLUGIN_BUILD_DIR}/${_ofx_BUNDLE_NAME}.ofx.bundle")
  set(_dest "${PLUGIN_INSTALLDIR}/${_ofx_BUNDLE_NAME}.ofx.bundle")
  set(PLUGIN_NAME "${_ofx_BUNDLE_NAME}")
  set(PLUGIN_EXE "${_ofx_BUNDLE_NAME}.ofx")
  set(BUNDLE_IDENTIFIER "${_ofx_IDENTIFIER}")
  if(NOT BUNDLE_IDENTIFIER)
    set(BUNDLE_IDENTIFIER "org.openeffects.plugins.${_ofx_BUNDLE_NAME}")
  endif()
  set(BUNDLE_SIGNATURE "????")
  set(PLUGIN_VERSION "${_ofx_VERSION}")
  if(NOT PLUGIN_VERSION)
    set(PLUGIN_VERSION "1.0.0")
  endif()
  set(_plist "${_ofx_INFO_PLIST}")
  if(NOT _plist)
    set(_plist "${_OFX_CMAKE_MODULE_DIR}/../Examples/Info.plist.in")
  endif()
  configure_file("${_plist}" "${_bundle}/Contents/Info.plist")
  set_target_properties(${TARGET} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${_bundle}/Contents/${ARCHDIR}"
    OFX_BUNDLE_DIR "${_bundle}" OFX_BUNDLE_INSTALL_DIR "${_dest}" OFX_BUNDLE_NAME "${_ofx_BUNDLE_NAME}")
  foreach(_cfg ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER "${_cfg}" _CFG)
    set_target_properties(${TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${_bundle}/Contents/${ARCHDIR}")
  endforeach()
  install(TARGETS ${TARGET} LIBRARY DESTINATION "${_dest}/Contents/${ARCHDIR}")
  install(FILES "${_bundle}/Contents/Info.plist" DESTINATION "${_dest}/Contents")
endfunction()

# ofx_plugin_resources(<target> [DESTINATION subdir] files...)
# Copy files into the bundle's Contents/Resources[/subdir], in the build tree and installed.
function(ofx_plugin_resources TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 _ofx "" "DESTINATION" "")
  get_target_property(_bundle ${TARGET} OFX_BUNDLE_DIR)
  get_target_property(_dest ${TARGET} OFX_BUNDLE_INSTALL_DIR)
  set(_sub "Contents/Resources")
  if(_ofx_DESTINATION)
    set(_sub "${_sub}/${_ofx_DESTINATION}")
  endif()
  set(_files "")
  foreach(_f ${_ofx_UNPARSED_ARGUMENTS})
    get_filename_component(_f "${_f}" ABSOLUTE)
    list(APPEND _files "${_f}")
  endforeach()
  if(NOT _files)
    return()
  endif()
  add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_bundle}/${_sub}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${_files} "${_bundle}/${_sub}"
    VERBATIM)
  install(FILES ${_files} DESTINATION "${_dest}/${_sub}")
endfunction()

# ofx_target_openmp(<target> [REQUIRED])
# Compile and link <target> with OpenMP if OFX_USE_OPENMP. Apple clang has no
# -fopenmp and no runtime: use "-Xpreprocessor -fopenmp" and MacPorts' libomp
# (<prefix>/include/libomp, <prefix>/lib/libomp), as Makefile.master.
function(ofx_target_openmp TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 _ofx "REQUIRED" "" "")
  if(NOT OFX_USE_OPENMP)
    return()
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND NOT OpenMP_CXX_FLAGS)
    find_path(OFX_LIBOMP_INCLUDE_DIR omp.h PATH_SUFFIXES libomp)
    find_library(OFX_LIBOMP_LIBRARY omp PATH_SUFFIXES libomp)
    if(OFX_LIBOMP_INCLUDE_DIR AND OFX_LIBOMP_LIBRARY)
      set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${OFX_LIBOMP_INCLUDE_DIR}" CACHE STRING "OpenMP flags (Apple clang)")
      set(OpenMP_CXX_LIB_NAMES "omp" CACHE STRING "OpenMP libraries (Apple clang)")
      set(OpenMP_omp_LIBRARY "${OFX_LIBOMP_LIBRARY}" CACHE FILEPATH "libomp (Apple clang)")
    endif()
  endif()
  if(_ofx_REQUIRED)
    find_package(OpenMP REQUIRED COMPONENTS CXX)
  else()
    find_package(OpenMP COMPONENTS CXX)
  endif()
  if(TARGET OpenMP::OpenMP_CXX)
    target_link_libraries(${TARGET} PRIVATE OpenMP::OpenMP_CXX)
  endif()
endfunction()

# ofx_deploy_bundle(<target>)
# If OFX_DEPLOY_BUNDLES: on macOS, run osxDeploy.sh on the installed bundle in Release
# and RelWithDebInfo, as Makefile.master does on its bundle (needs gsed). On Windows,
# copying the runtime DLLs is not implemented in this stand-in (the Makefiles don't).
function(ofx_deploy_bundle TARGET)
  if(NOT OFX_DEPLOY_BUNDLES OR NOT APPLE)
    return()
  endif()
  get_target_property(_dest ${TARGET} OFX_BUNDLE_INSTALL_DIR)
  get_target_property(_name ${TARGET} OFX_BUNDLE_NAME)
  set(_deploy "${_OFX_SUPPORT_INCLUDE_DIR}/osxDeploy.sh")
  install(CODE "
    if(CMAKE_INSTALL_CONFIG_NAME MATCHES \"^(Release|RelWithDebInfo)\$\")
      set(_b \"${_dest}\")
      if(NOT IS_ABSOLUTE \"\${_b}\")
        set(_b \"\${CMAKE_INSTALL_PREFIX}/\${_b}\")
      endif()
      message(STATUS \"osxDeploy.sh: \$ENV{DESTDIR}\${_b}\")
      execute_process(COMMAND bash \"${_deploy}\" \"\$ENV{DESTDIR}\${_b}\" \"${_name}.ofx\" RESULT_VARIABLE _r)
      if(_r)
        message(FATAL_ERROR \"osxDeploy.sh failed on \$ENV{DESTDIR}\${_b}\")
      endif()
    endif()")
endfunction()

# supportext_add_sources(<target> [ALL] [OPENGL] [OSMESA] [EGL] [COMPONENTS ...])
# Add SupportExt sources to <target>, so that they are built with its definitions;
# a short version of SupportExt's cmake/SupportExt.cmake (openfx-supportext #45),
# with its component names. Components that draw link OpenGL::GL.
if(NOT SUPPORTEXT_DIR)
  set(SUPPORTEXT_DIR "${PROJECT_SOURCE_DIR}/SupportExt")
endif()
set(_SE_THREADS tinythread.cpp ofxsThreadSuite.cpp)
set(_SE_FILEOPEN ofxsFileOpen.cpp)
set(_SE_LUT ofxsLut.cpp)
set(_SE_MULTIPLANE ofxsMultiPlane.cpp)
set(_SE_SHUTTER ofxsShutter.cpp)
set(_SE_TRANSFORM3X3 ofxsTransform3x3.cpp ofxsShutter.cpp)
set(_SE_TRANSFORM_INTERACT ofxsTransformInteract.cpp)
set(_SE_RECTANGLE_INTERACT ofxsRectangleInteract.cpp)
set(_SE_GENERATOR ofxsGenerator.cpp ofxsRectangleInteract.cpp)
set(_SE_TEXT ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp)
set(_SE_RAMP ofxsRamp.cpp ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp)
set(_SE_TRACKING ofxsTracking.cpp ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp)
set(_SE_OGL_FUNCTIONS ofxsOGLFunctions_gl.cpp glad/glad.cpp)
set(_SE_OPENGL ofxsOGLUtilities.cpp glad/glad.cpp ofxsOGLFunctions_gl.cpp ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp)
set(_SE_EGL glad/gladegl.cpp)
set(_SE_ALL THREADS FILEOPEN LUT MULTIPLANE SHUTTER TRANSFORM3X3 TRANSFORM_INTERACT RECTANGLE_INTERACT
    GENERATOR TEXT RAMP TRACKING OGL_FUNCTIONS OPENGL)
set(_SE_NO_GL THREADS FILEOPEN LUT MULTIPLANE SHUTTER TRANSFORM3X3 EGL)
function(supportext_add_sources TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 _se "ALL;OPENGL;OSMESA;EGL" "" "COMPONENTS")
  if(_se_OSMESA)
    message(FATAL_ERROR "supportext_add_sources: OSMESA is not in this stand-in")
  endif()
  set(_components ${_se_COMPONENTS})
  if(_se_ALL)
    list(APPEND _components ${_SE_ALL})
  endif()
  foreach(_opt OPENGL EGL)
    if(_se_${_opt})
      list(APPEND _components ${_opt})
    endif()
  endforeach()
  get_property(_files TARGET ${TARGET} PROPERTY _SUPPORTEXT_SOURCES)
  set(_new "")
  set(_gl OFF)
  foreach(_c ${_components})
    if(NOT DEFINED _SE_${_c} OR _c STREQUAL "ALL" OR _c STREQUAL "NO_GL")
      message(FATAL_ERROR "supportext_add_sources: unknown component ${_c}")
    endif()
    if(NOT _c IN_LIST _SE_NO_GL)
      set(_gl ON)
    endif()
    foreach(_f ${_SE_${_c}})
      if(NOT _f IN_LIST _files)
        list(APPEND _files ${_f})
        list(APPEND _new "${SUPPORTEXT_DIR}/${_f}")
      endif()
    endforeach()
  endforeach()
  set_property(TARGET ${TARGET} PROPERTY _SUPPORTEXT_SOURCES ${_files})
  if(_new)
    target_sources(${TARGET} PRIVATE ${_new})
  endif()
  target_include_directories(${TARGET} PRIVATE "${SUPPORTEXT_DIR}" "${SUPPORTEXT_DIR}/glad")
  if(_gl)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${TARGET} PRIVATE OpenGL::GL)
  endif()
endfunction()

# ioSupport_add_sources(<target>)
# Add the IOSupport sources (GenericReader, GenericOCIO, SequenceParsing) to <target>,
# so that they are built with its definitions (e.g. OFX_IO_USING_OCIO).
if(NOT IOSUPPORT_DIR)
  set(IOSUPPORT_DIR "${PROJECT_SOURCE_DIR}/OpenFX-IO/IOSupport")
endif()
function(ioSupport_add_sources TARGET)
  target_sources(${TARGET} PRIVATE
    "${IOSUPPORT_DIR}/GenericReader.cpp"
    "${IOSUPPORT_DIR}/GenericOCIO.cpp"
    "${IOSUPPORT_DIR}/SequenceParsing/SequenceParsing.cpp")
  target_include_directories(${TARGET} PRIVATE
    "${IOSUPPORT_DIR}"
    "${IOSUPPORT_DIR}/SequenceParsing"
    "${IOSUPPORT_DIR}/SequenceParsing/tinydir")
endfunction()
########################################################################
# end of the TODO block
########################################################################

# OpenGL: libGL on Linux, as Makefile.master (-lGL), not GLVND's libOpenGL
if(NOT DEFINED OpenGL_GL_PREFERENCE)
  set(OpenGL_GL_PREFERENCE LEGACY)
endif()

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
  arena_pkg_check_modules(POPPLER poppler-glib)
endif()
arena_pkg_check_modules(REVENGE librevenge-0.0 librevenge-generators-0.0 librevenge-stream-0.0)
arena_pkg_check_modules(CDR libcdr-0.1)
if(AUDIO)
  arena_pkg_check_modules(SOX sox)
endif()
if(HALDCLUT)
  # HaldCLUT uses libCurl to download ~100MiB of CLUT profiles
  arena_pkg_check_modules(CURL libcurl)
endif()

# OpenColorIO, for GenericReader's colorspace parameters, as Makefile.io (which,
# unlike Makefile.master, uses the shared link line)
pkg_check_modules(OCIO REQUIRED IMPORTED_TARGET OpenColorIO)
list(APPEND ARENA_LIBRARIES PkgConfig::OCIO)

# ImageMagick's .pc passes on the -fopenmp it was built with, which Apple clang
# rejects: drop it, and let ofx_target_openmp() add OpenMP (OFX_USE_OPENMP). Keep
# its OpenMP runtime (-lomp, -lgomp) in the link, which a static ImageMagick needs,
# and, as Makefile.master, find libomp next to ImageMagick (MacPorts: lib/libomp).
foreach(_prop INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_LIBRARIES)
  get_target_property(_flags PkgConfig::MAGICK ${_prop})
  if(_flags)
    list(REMOVE_ITEM _flags -fopenmp)
    set_property(TARGET PkgConfig::MAGICK PROPERTY ${_prop} "${_flags}")
  endif()
endforeach()
list(INSERT CMAKE_PREFIX_PATH 0 "${MAGICK_PREFIX}")
if(EXISTS "${MAGICK_PREFIX}/lib/libomp")
  set_property(TARGET PkgConfig::MAGICK APPEND PROPERTY INTERFACE_LINK_DIRECTORIES "${MAGICK_PREFIX}/lib/libomp")
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
if(AUDIO)
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
if(HALDCLUT)
    list(APPEND SOURCES Magick/HaldCLUT/HaldCLUT.cpp)
    list(APPEND RESOURCES
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.png
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.svg
        Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.xml)
endif()

add_ofx_plugin(Arena INFO_PLIST Bundle/Info.plist SOURCES ${SOURCES})
supportext_add_sources(Arena COMPONENTS TEXT RECTANGLE_INTERACT TRANSFORM3X3 TRANSFORM_INTERACT MULTIPLANE)
ioSupport_add_sources(Arena)
target_include_directories(Arena PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Magick"
    "${CMAKE_CURRENT_SOURCE_DIR}/Extra"
    "${CMAKE_CURRENT_SOURCE_DIR}/Audio"
    "${CMAKE_CURRENT_SOURCE_DIR}/Bundle"
    "${CMAKE_CURRENT_SOURCE_DIR}/Text"
    "${CMAKE_CURRENT_SOURCE_DIR}/lodepng")
target_link_libraries(Arena PRIVATE ${ARENA_LIBRARIES})
ofx_target_openmp(Arena)
target_compile_definitions(Arena PRIVATE OFX_IO_USING_OCIO)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # as Makefile.io
    set_property(TARGET Arena APPEND PROPERTY INSTALL_RPATH "${OCIO_LIBDIR}")
endif()
if(LEGACYIM)
    target_compile_definitions(Arena PRIVATE LEGACYIM NOMAGICKSEED)
elseif(IM STREQUAL "68")
    target_compile_definitions(Arena PRIVATE NOMAGICKSEED)
endif()
if(READMISC_LEGACY)
    target_compile_definitions(Arena PRIVATE READMISC_LEGACY)
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
