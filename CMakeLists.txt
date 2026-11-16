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
#   supportext_add_sources(<target> [ALL] [OPENGL] [OSMESA] [COMPONENTS ...])
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

# supportext_add_sources(<target> [ALL] [OPENGL] [OSMESA] [COMPONENTS ...])
# Add SupportExt sources to <target>, so that they are built with its definitions.
# OPENGL: the OpenGL text renderer. Interacts and OPENGL link OpenGL (libGL on Linux,
# as Makefile.master).
if(NOT SUPPORTEXT_DIR)
  set(SUPPORTEXT_DIR "${PROJECT_SOURCE_DIR}/SupportExt")
endif()
function(supportext_add_sources TARGET)
  cmake_parse_arguments(PARSE_ARGV 1 _se "ALL;OPENGL;OSMESA" "" "COMPONENTS")
  if(_se_OSMESA)
    message(FATAL_ERROR "supportext_add_sources: OSMESA is not in this stand-in")
  endif()
  set(_TRANSFORM ofxsTransform3x3.cpp ofxsTransformInteract.cpp ofxsShutter.cpp)
  set(_SHUTTER ofxsShutter.cpp)
  set(_RECTANGLE ofxsRectangleInteract.cpp)
  set(_MULTIPLANE ofxsMultiPlane.cpp)
  set(_LUT ofxsLut.cpp)
  set(_GENERATOR ofxsGenerator.cpp)
  set(_RAMP ofxsRamp.cpp)
  set(_TRACKING ofxsTracking.cpp)
  set(_FILEOPEN ofxsFileOpen.cpp)
  set(_THREAD tinythread.cpp ofxsThreadSuite.cpp)
  set(_components ${_se_COMPONENTS})
  if(_se_ALL)
    set(_components TRANSFORM RECTANGLE MULTIPLANE LUT GENERATOR RAMP TRACKING FILEOPEN THREAD)
    set(_se_OPENGL ON)
  endif()
  set(_files "")
  foreach(_c ${_components})
    if(NOT DEFINED _${_c})
      message(FATAL_ERROR "supportext_add_sources: unknown component ${_c}")
    endif()
    list(APPEND _files ${_${_c}})
  endforeach()
  if(_se_OPENGL)
    list(APPEND _files ofxsOGLTextRenderer.cpp ofxsOGLFontData.cpp)
  endif()
  list(REMOVE_DUPLICATES _files)
  list(TRANSFORM _files PREPEND "${SUPPORTEXT_DIR}/")
  target_sources(${TARGET} PRIVATE ${_files})
  target_include_directories(${TARGET} PRIVATE "${SUPPORTEXT_DIR}")
  if(_se_OPENGL OR _components)
    if(NOT DEFINED OpenGL_GL_PREFERENCE)
      set(OpenGL_GL_PREFERENCE LEGACY)
    endif()
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
arena_pkg_check_modules(POPPLER poppler-glib)
arena_pkg_check_modules(REVENGE librevenge-0.0 librevenge-generators-0.0 librevenge-stream-0.0)
arena_pkg_check_modules(CDR libcdr-0.1)
if(AUDIO)
  arena_pkg_check_modules(SOX sox)
endif()
if(HALDCLUT)
  # HaldCLUT uses libCurl to download ~100MiB of CLUT profiles
  arena_pkg_check_modules(CURL libcurl)
endif()

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

if(AUDIO)
    list(APPEND SOURCES "Audio/AudioCurve.cpp")
    list(APPEND RESOURCES "Audio/net.fxarena.openfx.AudioCurve.png")
endif()

if(RICHTEXT)
    list(APPEND SOURCES "Text/RichTextOFX.cpp")
    list(
        APPEND
        RESOURCES
        "Text/net.fxarena.openfx.RichText.png"
        "Text/net.fxarena.openfx.RichText.svg")
endif()

if(HALDCLUT)
    list(APPEND SOURCES "Magick/HaldCLUT/HaldCLUT.cpp")
    list(
        APPEND
        RESOURCES
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.png"
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.svg"
        "Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.xml")
endif()

add_ofx_plugin(Arena INFO_PLIST Bundle/Info.plist SOURCES ${SOURCES})
supportext_add_sources(Arena OPENGL COMPONENTS TRANSFORM RECTANGLE MULTIPLANE)
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
if(READMISC_LEGACY)
    target_compile_definitions(Arena PRIVATE READMISC_LEGACY)
endif()
if(OLD_POPPLER)
    target_compile_definitions(Arena PRIVATE LEGACY_POPPLER)
endif()

# ReadPDF.cpp includes poppler's C++ headers (GlobalParams.h), which need
# C++17 since poppler 21.12 (#23) and C++20 (std::span) in recent versions.
# Only that file is built with this standard (see also Makefile.master).
# gnu++2a: MinGW GCC 16 headers need GNU extensions in C++20 mode, and
# GCC < 10 only knows the "2a" spelling.
if(MSVC)
    set(POPPLER_CXXSTD_FLAG "/std:c++20")
else()
    set(POPPLER_CXXSTD_FLAG "-std=gnu++2a")
endif()
set_source_files_properties(Extra/ReadPDF.cpp PROPERTIES COMPILE_FLAGS "${POPPLER_CXXSTD_FLAG}")

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
