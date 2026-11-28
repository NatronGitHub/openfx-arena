# ReadSVG's DPI: the output size of SVG documents in physical units (mm) and in user units (px),
# rendered with openfx's hostDemo at several DPIs, by both major versions. Version 4 applies the
# DPI once; version 3 (Natron 2.5.0) applied it twice to physical units, and old projects keep that
# (#98; the cases are in tests/regression/readsvg).
#
#   cmake -DHOSTDEMO=<hostDemo> -DPLUGIN_PATH=<dir with Arena.ofx.bundle> -DSRC=<tests/regression/readsvg>
#         -DOCIO=<config.ocio> -DWORK=<dir> [-DEXACT=ON] [-DCASES=<files>] -P readsvg_dpi.cmake
#
# EXACT (librsvg 2.52 or later): ReadSVG 4 then has the exact sizes and renders a document in
# millimetres exactly as the same document with a viewBox in user units. With older versions of
# librsvg, ReadSVG 4 takes the size in physical units from librsvg at the DPI, in whole pixels: it
# is allowed to be 1 pixel off, for librsvg's own rounding, at any DPI (multiplying the size
# rounded at 90 DPI by dpi / 90 would be up to dpi / 180 pixels off, e.g. 4 at 1200 DPI).
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt (string(JSON) needs 3.19)
foreach(_v HOSTDEMO PLUGIN_PATH SRC OCIO WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "readsvg_dpi.cmake: ${_v} is not set")
  endif()
  get_filename_component(${_v} "${${_v}}" ABSOLUTE)
endforeach()

include("${CMAKE_CURRENT_LIST_DIR}/testenv.cmake") # native(), test_env(), check_error_logs()

test_env("${WORK}" "${PLUGIN_PATH}" "${OCIO}")

set(_failed 0)
# render <major> <file> <dpi>: sets _w, _h (of every view, which must agree) and _crc
macro(render _major _file _dpi)
  set(_prefix "${WORK}/${_file}-v${_major}-${_dpi}")
  native(_nprefix "${_prefix}")
  native(_nfile "${SRC}/${_file}.svg")
  # each run its own debug log, so that an ERROR line points at the run that wrote it
  set(ENV{OFX_PLUGIN_LOGFILE} "${_nprefix}.ofxTestLog.txt")
  execute_process(
    COMMAND "${HOSTDEMO}" --quiet --no-std-paths --no-cache -p net.fxarena.openfx.ReadSVG -m ${_major}
            -c general --frames 0 -o "${_nprefix}" --report "${_nprefix}.json"
            -s "filename=${_nfile}" -s "dpi=${_dpi}"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
    TIMEOUT 300)
  set(_w "")
  set(_h "")
  set(_crc "")
  if(NOT "${_r}" STREQUAL "0")
    message("FAIL ReadSVG ${_major} ${_file}.svg at ${_dpi} DPI: ${_r}\n${_log}")
    math(EXPR _failed "${_failed} + 1")
  else()
    file(READ "${_prefix}.json" _json)
    string(JSON _n LENGTH "${_json}" renders)
    math(EXPR _last "${_n} - 1")
    foreach(_i RANGE ${_last})
      string(JSON _rw GET "${_json}" renders ${_i} output w)
      string(JSON _rh GET "${_json}" renders ${_i} output h)
      string(JSON _rcrc GET "${_json}" renders ${_i} output crc32)
      if(_i EQUAL 0)
        set(_w ${_rw})
        set(_h ${_rh})
        set(_crc ${_rcrc})
      elseif(NOT _rw EQUAL _w OR NOT _rh EQUAL _h OR NOT _rcrc STREQUAL _crc)
        message("FAIL ReadSVG ${_major} ${_file}.svg at ${_dpi} DPI: the views differ")
        math(EXPR _failed "${_failed} + 1")
      endif()
    endforeach()
  endif()
  # the PPM files are large at high DPIs
  file(GLOB _outputs "${_prefix}.*.ppm")
  if(_outputs)
    file(REMOVE ${_outputs})
  endif()
endmacro()

# check <major> <file> <dpi> <width> <height> <tolerance in pixels>
macro(check _major _file _dpi _ew _eh _tol)
  render(${_major} ${_file} ${_dpi})
  if(NOT _w STREQUAL "")
    math(EXPR _dw "${_w} - ${_ew}")
    math(EXPR _dh "${_h} - ${_eh}")
    if(_dw LESS -${_tol} OR _dw GREATER ${_tol} OR _dh LESS -${_tol} OR _dh GREATER ${_tol})
      message("FAIL ReadSVG ${_major} ${_file}.svg at ${_dpi} DPI: ${_w}x${_h}, expected ${_ew}x${_eh}")
      math(EXPR _failed "${_failed} + 1")
    else()
      message("ok   ReadSVG ${_major} ${_file}.svg at ${_dpi} DPI: ${_w}x${_h}")
    endif()
  endif()
endmacro()

# the cases: check() and render() calls, from the case list(s) in CASES (by default regression/)
if(NOT DEFINED CASES)
  set(CASES "${CMAKE_CURRENT_LIST_DIR}/../regression/readsvg/cases.cmake")
endif()
foreach(_c IN LISTS CASES)
  get_filename_component(_c "${_c}" ABSOLUTE)
  include("${_c}")
endforeach()

# the ERROR lines of a debug build's property validation
check_error_logs("${WORK}" _bad)
list(LENGTH _bad _nbad)
math(EXPR _failed "${_failed} + ${_nbad}")
if(_failed GREATER 0)
  message(FATAL_ERROR "ReadSVG DPI test: ${_failed} failed")
endif()
