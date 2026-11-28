# Render the readers with render windows smaller than the image (#105), or partly or entirely
# outside of it (#108), with reader_window (reader_window.cpp), at scale 1 and 0.5: each window
# must give the same pixels as the same region of a full render, black outside of the RoD, and
# the reader must write only the window.
#
#   cmake -DREADER_WINDOW=<reader_window> -DPLUGIN_PATH=<dir with Arena.ofx.bundle>
#         -DSAMPLES=<dir> -DOCIO=<config.ocio> -DWORK=<dir> [-DREADPDF=ON] [-DCASES=<files>]
#         -P reader_window.cmake
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

# the cases: check() calls, from the case lists in CASES (by default the functional and the
# regression ones)
if(NOT DEFINED CASES)
  set(CASES "${CMAKE_CURRENT_LIST_DIR}/../functional/readerwindow/cases.cmake"
            "${CMAKE_CURRENT_LIST_DIR}/../regression/readerwindow/cases.cmake")
endif()
foreach(_c IN LISTS CASES)
  get_filename_component(_c "${_c}" ABSOLUTE)
  include("${_c}")
endforeach()

# the ERROR lines of a debug build's property validation
check_error_logs("${WORK}" _bad)
list(APPEND _failed ${_bad})
list(LENGTH _failed _nf)
message("${_ok} passed, ${_nf} failed")
if(_failed)
  message(FATAL_ERROR "reader window test failed: ${_failed}")
endif()
