# ReadSVG's DPI: the output size of SVG documents in physical units (mm) and in user units (px),
# rendered with openfx's hostDemo at several DPIs, by both major versions. Version 4 applies the
# DPI once; version 3 (Natron 2.5.0) applied it twice to physical units, and old projects keep that.
#
#   cmake -DHOSTDEMO=<hostDemo> -DPLUGIN_PATH=<dir with Arena.ofx.bundle> -DSRC=<tests/regression/readsvg>
#         -DOCIO=<config.ocio> -DWORK=<dir> [-DEXACT=ON] -P readsvg_dpi.cmake
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

macro(native _out _path)
  if(CMAKE_HOST_WIN32)
    file(TO_NATIVE_PATH "${_path}" ${_out})
  else()
    set(${_out} "${_path}")
  endif()
endmacro()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
native(_n "${PLUGIN_PATH}")
set(ENV{OFX_PLUGIN_PATH} "${_n}")
native(_n "${OCIO}")
set(ENV{OCIO} "${_n}")
native(_n "${WORK}")
set(ENV{HOME} "${_n}")
set(ENV{USERPROFILE} "${_n}")

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

if(EXACT)
  set(_tol 0)
else()
  set(_tol 1)
endif()
# 100mm x 50mm is 354.33 x 177.17 pixels at 90 DPI: the size is proportional to the DPI
check(4 mm 90 354 177 ${_tol})
check(4 mm 180 709 354 ${_tol})
check(4 mm 300 1181 591 ${_tol})
check(4 mm 600 2362 1181 ${_tol})
check(4 mm 1200 4724 2362 ${_tol})
check(4 mm_units 180 709 354 ${_tol})
set(_crc_mm_units "${_crc}")
check(4 mm_units 1200 4724 2362 ${_tol})
# user units: 400 x 200 at 90 DPI, scaled by dpi / 90, rounded to the nearest pixel
check(4 px 90 400 200 0)
check(4 px 180 800 400 0)
check(4 px 300 1333 667 0)
# version 3: the DPI applied twice to physical units (4 times the size at 180 DPI), and the size
# truncated
check(3 mm 90 354 177 ${_tol})
check(3 mm 180 1418 708 ${_tol})
check(3 px 180 800 400 0)
check(3 px 300 1333 666 0)
if(EXACT)
  # the same document in millimetres, and with a viewBox in user units: the same image
  render(4 mm 180)
  if(NOT _crc STREQUAL "" AND NOT _crc STREQUAL _crc_mm_units)
    message("FAIL ReadSVG 4 at 180 DPI: mm.svg and mm_units.svg differ (crc32 ${_crc} and ${_crc_mm_units})")
    math(EXPR _failed "${_failed} + 1")
  else()
    message("ok   ReadSVG 4 at 180 DPI: mm.svg and mm_units.svg are the same image")
  endif()
endif()
# a debug build's property validation (Support's ofxsLog.cpp) writes an "ERROR : " line in the
# log of each run for a host property that is missing or wrong
file(GLOB _logs "${WORK}/*ofxTestLog.txt")
foreach(_l IN LISTS _logs)
  file(STRINGS "${_l}" _errors REGEX "ERROR : ")
  if(_errors)
    list(JOIN _errors "\n" _errors)
    message("FAIL ${_l}:\n${_errors}")
    math(EXPR _failed "${_failed} + 1")
  endif()
endforeach()
if(_failed GREATER 0)
  message(FATAL_ERROR "ReadSVG DPI test: ${_failed} failed")
endif()
