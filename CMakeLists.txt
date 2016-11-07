cmake_minimum_required(VERSION 2.8) 

project(openfx-arena)

# Check for OpenFX
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/OpenFX/include)
  message(FATAL_ERROR "\n OpenFX is missing, please update your repository.\n")
endif()

# Add extra cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# OfxSupport
add_subdirectory("${PROJECT_SOURCE_DIR}/OpenFX")
include_directories("${CMAKE_SOURCE_DIR}/OpenFX/Support/Plugins/include")
target_compile_definitions(OfxSupport PUBLIC OFX_EXTENSIONS_VEGAS OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_NATRON OFX_SUPPORTS_OPENGLRENDER)

# SupportExt
include_directories("${CMAKE_SOURCE_DIR}/SupportExt")

# Text depends
find_package(PangoCairo)
find_package(Fontconfig)
find_package(Glib)

# Create Text plugin
create_ofx_plugin(TextFX 
    "TextFX" 
    "${PangoCairo_LIBRARIES}" 
    "${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.png;${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.svg" 
    "${PangoCairo_INCLUDE_DIR};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
    "${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransform3x3.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransformInteract.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsShutter.cpp")

# IO depends
find_package(OpenColorIO)
find_package(LibRSVG)
find_package(LibXml2)

# Create IO plugins
add_definitions(-DOFX_IO_USING_OCIO)
include_directories("${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport" "${CMAKE_SOURCE_DIR}/SupportExt/glad")

create_ofx_plugin(ReadSVG 
    "ReadSVG" 
    "${OCIO_LIBRARIES};${LibRSVG_LIBRARIES};${LIBXML2_LIBRARIES}" 
    "${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.png;${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.svg" 
    "${OCIO_INCLUDES};${LIBXML2_INCLUDE_DIR};${LibRSVG_INCLUDE_DIRS};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp")

