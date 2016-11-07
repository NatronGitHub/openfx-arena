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

# IOSupport
add_definitions(-DOFX_IO_USING_OCIO)
include_directories("${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport" "${CMAKE_SOURCE_DIR}/SupportExt/glad" "${PROJECT_SOURCE_DIR}")

# Find packages
#add_definitions(-DMAGICKCORE_QUANTUM_DEPTH=32)
#add_definitions(-DMAGICKCORE_HDRI_ENABLE=1)
find_package(PangoCairo)
find_package(Fontconfig)
find_package(Glib)
find_package(OpenColorIO)
find_package(LibRSVG)
find_package(LibXml2)
find_package(PopplerCairo)
#find_package(LibCDR)
#find_package(LibRevenge)
find_package(LCMS2)
find_package(ImageMagick COMPONENTS Magick++)

# lodepng
set(PNGVERSION "a70c086077c0eaecbae3845e4da4424de5f43361")
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/lodepng.h)
  execute_process(COMMAND curl https://raw.githubusercontent.com/lvandeve/lodepng/${PNGVERSION}/lodepng.h -o ${PROJECT_SOURCE_DIR}/lodepng.h)
endif()
if(NOT EXISTS ${PROJECT_SOURCE_DIR}/lodepng.cpp)
  execute_process(COMMAND curl https://raw.githubusercontent.com/lvandeve/lodepng/${PNGVERSION}/lodepng.cpp -o ${PROJECT_SOURCE_DIR}/lodepng.cpp)
endif()

# Text.ofx
#create_ofx_plugin(TextFX 
#    "TextFX" 
#    "${PangoCairo_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.png;${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.svg" 
#    "${PangoCairo_INCLUDE_DIR};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
#    "${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransform3x3.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransformInteract.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsShutter.cpp")

# ReadSVG.ofx
#create_ofx_plugin(ReadSVG 
#    "ReadSVG" 
#    "${OCIO_LIBRARIES};${LibRSVG_LIBRARIES};${LIBXML2_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/ReadSVG/net.fxarena.openfx.ReadSVG.png;${PROJECT_SOURCE_DIR}/ReadSVG/net.fxarena.openfx.ReadSVG.svg" 
#    "${OCIO_INCLUDES};${LIBXML2_INCLUDE_DIR};${LibRSVG_INCLUDE_DIRS};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
#    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp")

# ReadPDF.ofx
#create_ofx_plugin(ReadPDF 
#    "ReadPDF" 
#    "${OCIO_LIBRARIES};${POPPLER_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/ReadPDF/fr.inria.openfx.ReadPDF.png;${PROJECT_SOURCE_DIR}/ReadPDF/fr.inria.openfx.ReadPDF.svg" 
#    "${OCIO_INCLUDES};${POPPLER_INCLUDE_DIRS};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
#    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp")

# ReadCDR.ofx
#create_ofx_plugin(ReadCDR 
#    "ReadCDR" 
#    "${OCIO_LIBRARIES};${LIBCDR_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/ReadCDR/fr.inria.openfx.ReadCDR.png;${PROJECT_SOURCE_DIR}/ReadCDR/fr.inria.openfx.ReadCDR.svg" 
#    "${OCIO_INCLUDES};${LIBCDR_INCLUDE_DIRS};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS}"
#    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp")

# OpenRaster.ofx
#create_ofx_plugin(OpenRaster 
#    "OpenRaster" 
#    "${OCIO_LIBRARIES};${LIBXML2_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/OpenRaster/fr.inria.openfx.OpenRaster.png;${PROJECT_SOURCE_DIR}/OpenRaster/fr.inria.openfx.OpenRaster.svg" 
#    "${OCIO_INCLUDES};${LIBXML2_INCLUDE_DIR}"
#    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp;${CMAKE_SOURCE_DIR}/lodepng.cpp")

# ReadKrita.ofx
#create_ofx_plugin(ReadKrita 
#    "ReadKrita" 
#    "${OCIO_LIBRARIES};${LIBXML2_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/ReadKrita/fr.inria.openfx.ReadKrita.png;${PROJECT_SOURCE_DIR}/ReadKrita/fr.inria.openfx.ReadKrita.svg" 
#    "${OCIO_INCLUDES};${LIBXML2_INCLUDE_DIR}"
#    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp;${CMAKE_SOURCE_DIR}/lodepng.cpp")

# Extra.ofx (bundle)
#create_ofx_plugin(Extra 
#    "TextFX;ReadSVG;ReadPDF;OpenRaster;ReadKrita" 
#    "${PangoCairo_LIBRARIES};${OCIO_LIBRARIES};${LibRSVG_LIBRARIES};${LIBXML2_LIBRARIES};${POPPLER_LIBRARIES}" 
#    "${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.png;${PROJECT_SOURCE_DIR}/TextFX/net.fxarena.openfx.Text.svg;${PROJECT_SOURCE_DIR}/ReadSVG/net.fxarena.openfx.ReadSVG.png;${PROJECT_SOURCE_DIR}/ReadSVG/net.fxarena.openfx.ReadSVG.svg;${PROJECT_SOURCE_DIR}/ReadPDF/fr.inria.openfx.ReadPDF.png;${PROJECT_SOURCE_DIR}/ReadPDF/fr.inria.openfx.ReadPDF.svg;${PROJECT_SOURCE_DIR}/OpenRaster/fr.inria.openfx.OpenRaster.png;${PROJECT_SOURCE_DIR}/OpenRaster/fr.inria.openfx.OpenRaster.svg;${PROJECT_SOURCE_DIR}/ReadKrita/fr.inria.openfx.ReadKrita.png;${PROJECT_SOURCE_DIR}/ReadKrita/fr.inria.openfx.ReadKrita.svg" 
#    "${PangoCairo_INCLUDE_DIR};${Glib_INCLUDE_DIR};${GlibConfig_INCLUDE_DIR};${CAIRO_INCLUDE_DIRS};${FONTCONFIG_INCLUDE_DIR};${FREETYPE_INCLUDE_DIRS};${OCIO_INCLUDES};${LIBXML2_INCLUDE_DIR};${LibRSVG_INCLUDE_DIRS};${POPPLER_INCLUDE_DIRS}"
#    "${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransform3x3.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsTransformInteract.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsShutter.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp;${CMAKE_SOURCE_DIR}/lodepng.cpp")

# ReadPSD.ofx
create_ofx_plugin(ReadPSD 
    "ReadPSD" 
    "${OCIO_LIBRARIES};${ImageMagick_LIBRARIES};${LCMS2_LIBRARIES}" 
    "${PROJECT_SOURCE_DIR}/ReadPSD/net.fxarena.openfx.ReadPSD.png;${PROJECT_SOURCE_DIR}/ReadPSD/net.fxarena.openfx.ReadPSD.svg" 
    "${OCIO_INCLUDES};${ImageMagick_INCLUDE_DIRS};${LCMS2_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericReader.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIO.cpp;${CMAKE_SOURCE_DIR}/OpenFX-IO/IOSupport/GenericOCIOOpenGL.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsMultiPlane.cpp;${CMAKE_SOURCE_DIR}/SupportExt/ofxsFileOpen.cpp;${CMAKE_SOURCE_DIR}/ofxsOGLUtilities.cpp")


