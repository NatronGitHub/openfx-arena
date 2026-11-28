# Render the readers with render windows smaller than the image (#105), or partly or entirely
# outside of it (#108), with reader_window (reader_window.cpp), at scale 1 and 0.5: each window
# must give the same pixels as the same region of a full render, black outside of the RoD, and
# the reader must write only the window.
#
#   cmake -DREADER_WINDOW=<reader_window> -DPLUGIN_PATH=<dir with Arena.ofx.bundle>
#         -DSAMPLES=<dir> -DOCIO=<config.ocio> -DWORK=<dir> [-DREADPDF=ON] -P reader_window.cmake
#
# READPDF: the bundle has ReadPDF (not with OFX_LICENSE=COMMERCIAL).
# SAMPLES has the files of make_samples.cpp (and sample.svg), including the sample_xy files.
# There is no CorelDRAW sample (see tests/functional/hostdemo/arena.txt), so ReadCDR is not
# tested here.
#
# Each reader is rendered twice: with its default parameters, and with another filePremult, so
# that both paths of GenericReader::render() are tested: decode() straight into the output image
# (its bounds are the RoD, the render window is a part of it), and decode() into a temporary
# image that covers just the render window (when the colours must be premultiplied).
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt
foreach(_v READER_WINDOW PLUGIN_PATH SAMPLES OCIO WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "reader_window.cmake: ${_v} is not set")
  endif()
  get_filename_component(${_v} "${${_v}}" ABSOLUTE)
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/testenv.cmake") # native(), test_env(), check_error_logs()

test_env("${WORK}" "${PLUGIN_PATH}" "${OCIO}")
# the caches in the work directory, which is wiped at each run
native(_n "${WORK}")
set(ENV{XDG_CACHE_HOME} "${_n}")

set(_ok 0)
set(_failed "")
set(_n_run 0)
# check(<identifier> <major> <sample> [name=value...])
macro(check _id _major _sample)
  native(_file "${SAMPLES}/${_sample}")
  # each run its own debug log, so that an ERROR line points at the run that wrote it
  math(EXPR _n_run "${_n_run} + 1")
  native(_nlog "${WORK}/run${_n_run}.ofxTestLog.txt")
  set(ENV{OFX_PLUGIN_LOGFILE} "${_nlog}")
  execute_process(
    COMMAND "${READER_WINDOW}" ${_id} ${_major} "filename=${_file}" ${ARGN}
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
    TIMEOUT 300)
  string(REPLACE ";" " " _args "${ARGN}")
  if("${_r}" STREQUAL "0")
    message("ok   ${_id} v${_major} ${_sample} ${_args}")
    math(EXPR _ok "${_ok} + 1")
  else()
    message("FAIL ${_id} v${_major} ${_sample} ${_args}: ${_r}\n${_log}")
    list(APPEND _failed "${_id}v${_major}")
  endif()
endmacro()

# filePremult: 1 premultiplied, 2 unpremultiplied (the readers are RGBA only, so the menu is
# Opaque, PreMultiplied, UnPreMultiplied). The default of ReadSVG is premultiplied (decode()
# into the output image), the others' is unpremultiplied (decode() into a temporary image).
check(net.fxarena.openfx.ReadSVG 4 sample.svg)
check(net.fxarena.openfx.ReadSVG 4 sample.svg filePremult=2)
check(net.fxarena.openfx.ReadSVG 4 sample.svg dpi=150)
# the transform: the whole transformed canvas is rendered, whatever the window
check(net.fxarena.openfx.ReadSVG 4 sample.svg transformRotate=20)
check(net.fxarena.openfx.ReadSVG 3 sample.svg)
check(net.fxarena.openfx.ReadSVG 3 sample.svg filePremult=2)
check(net.fxarena.openfx.ReadSVG 3 sample.svg dpi=150)
# the sample_xy files differ along x and y (the other samples are a vertical gradient)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 sample.pdf)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf filePremult=1)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf dpi=100)
endif()
check(fr.inria.openfx.ReadKrita 2 sample.kra)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra filePremult=1)
check(fr.inria.openfx.OpenRaster 2 sample.ora)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora filePremult=1)
# a PNG smaller than the image size in the XML: anchored at the top-left corner, black around it
# (#108; it used to fail). --placement checks where it lands: sample_xy's image, 64x48, at
# (0,12) in the 80x60 RoD; and the OpenRaster layer at x=10, y=5 from the top-left corner, at
# (10,60-5-48=7)
check(fr.inria.openfx.ReadKrita 2 sample_big.kra --placement "${SAMPLES}/sample_xy.kra" 0 12)
check(fr.inria.openfx.OpenRaster 2 sample_big.ora --placement "${SAMPLES}/sample_xy.ora" 0 12)
check(fr.inria.openfx.OpenRaster 2 sample_offset.ora --placement "${SAMPLES}/sample_xy.ora" 10 7)
check(fr.inria.openfx.OpenRaster 2 sample_offset.ora filePremult=1 --placement "${SAMPLES}/sample_xy.ora" 10 7)
# a layer without x and y (optional in stack.xml, 0 by default, #119): at the top-left corner
check(fr.inria.openfx.OpenRaster 2 sample_noxy.ora --placement "${SAMPLES}/sample_xy.ora" 0 12)
# ReadPSD and ReadMisc already copied just the window
check(net.fxarena.openfx.ReadPSD 2 sample.psd)
check(fr.inria.openfx.ReadMisc 1 sample.bmp)
# at half scale (a 50% viewer, proxy mode): decode() gets a render scale of 1 and the window at
# full resolution, and GenericReader downscales it (#108); both paths of render(), as above
check(net.fxarena.openfx.ReadSVG 4 sample.svg --scale 0.5)
check(net.fxarena.openfx.ReadSVG 4 sample.svg filePremult=2 --scale 0.5)
check(net.fxarena.openfx.ReadSVG 3 sample.svg --scale 0.5)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf --scale 0.5)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf filePremult=1 --scale 0.5)
endif()
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra --scale 0.5)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra filePremult=1 --scale 0.5)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora --scale 0.5)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora filePremult=1 --scale 0.5)
check(net.fxarena.openfx.ReadPSD 2 sample.psd --scale 0.5)
check(fr.inria.openfx.ReadMisc 1 sample.bmp --scale 0.5)

# the ERROR lines of a debug build's property validation
check_error_logs("${WORK}" _bad)
list(APPEND _failed ${_bad})
list(LENGTH _failed _nf)
message("${_ok} passed, ${_nf} failed")
if(_failed)
  message(FATAL_ERROR "reader window test failed: ${_failed}")
endif()
