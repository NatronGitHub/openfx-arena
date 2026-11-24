# Render the readers on malformed or missing files with openfx's hostDemo: each must fail cleanly,
# with an error status and the reader's message, not crash, hang, or render garbage.
#
#   cmake -DHOSTDEMO=<hostDemo> -DPLUGIN_PATH=<dir with Arena.ofx.bundle> -DSAMPLES=<dir>
#         -DOCIO=<config.ocio> -DWORK=<dir> -P run.cmake
#
# SAMPLES has the files of tests/hostdemo, including the bad_ files that make_samples.cpp writes:
# - bad_nosize.kra/.ora: no image size in the XML. getFrameBounds() returned true with the bounds
#   unset, and GenericReader made the RoD from an uninitialized rectangle (#129).
# - bad_hugexml, bad_hugepng: an XML or PNG entry whose size in the archive (2^64-1 or 2^63-1) is
#   far larger than its contents. The readers allocated that size (new char[size + 1] wrapped
#   around to new char[0] for 2^64-1, then zip_fread() wrote past it) (#128).
# - bad_hugecanvas.ora: a 200000000x200000000 canvas in stack.xml, which the host would allocate (#214).
# - bad_pngbomb.kra: a merged image whose PNG header says 65535x65535 (17 GB of RGBA) (#159).
# - bad_huge.svg: a canvas too large for an int or for cairo (#158).
# - bad_corrupt: not a zip file (or BMP file) at all; bad_empty.svg: an empty canvas (#129);
#   missing.*: no such file.
# A failure is an exit status other than 0 (a crash is reported by execute_process() as a string,
# e.g. "Segmentation fault", and fails the test), with no output frame, within the timeout.
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt
foreach(_v HOSTDEMO PLUGIN_PATH SAMPLES OCIO WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "run.cmake: ${_v} is not set")
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
set(ENV{XDG_CACHE_HOME} "${_n}")

set(_ok 0)
set(_failed "")
set(_n_run 0)
# fails(<identifier> <major> <file> <regex of the expected message>)
macro(fails _id _major _file _regex)
  math(EXPR _n_run "${_n_run} + 1")
  set(_prefix "${WORK}/run${_n_run}")
  native(_nfile "${SAMPLES}/${_file}")
  native(_nprefix "${_prefix}")
  # each run its own debug log: the default ofxTestLog.txt is opened with "w" by each run
  set(ENV{OFX_PLUGIN_LOGFILE} "${_nprefix}.ofxTestLog.txt")
  execute_process(
    COMMAND "${HOSTDEMO}" --quiet --no-std-paths --no-cache -p ${_id} -m ${_major} -c general
            --frames 0 -o "${_nprefix}" -s "filename=${_nfile}"
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
    TIMEOUT 120)
  file(GLOB _outputs "${_prefix}.*.ppm")
  if(_outputs)
    file(REMOVE ${_outputs})
  endif()
  if(NOT "${_r}" MATCHES "^[0-9]+$")
    set(_why "crashed or timed out (${_r})")
  elseif("${_r}" STREQUAL "0")
    set(_why "succeeded")
  elseif(_outputs)
    set(_why "failed (${_r}), but wrote an output frame")
  elseif(NOT "${_log}" MATCHES "${_regex}")
    set(_why "failed (${_r}), without the message \"${_regex}\"")
  else()
    set(_why "")
  endif()
  if(_why STREQUAL "")
    message("ok   ${_id} v${_major} ${_file}")
    math(EXPR _ok "${_ok} + 1")
  else()
    message("FAIL ${_id} v${_major} ${_file}: ${_why}\n${_log}")
    list(APPEND _failed "${_id}v${_major}:${_file}")
  endif()
endmacro()

fails(fr.inria.openfx.ReadKrita 2 bad_nosize.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_hugexml.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_hugepng.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_pngbomb.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 bad_corrupt.kra "Unable to read image")
fails(fr.inria.openfx.ReadKrita 2 missing.kra "Cannot load frame|No filename")
fails(fr.inria.openfx.OpenRaster 2 bad_nosize.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugecanvas.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugexml.ora "Unable to read image|Empty and/or corrupt image")
fails(fr.inria.openfx.OpenRaster 2 bad_hugepng.ora "Unable to read image")
fails(fr.inria.openfx.OpenRaster 2 bad_corrupt.ora "Unable to read image|Empty and/or corrupt image")
fails(fr.inria.openfx.OpenRaster 2 missing.ora "Cannot load frame|No filename")
fails(net.fxarena.openfx.ReadSVG 4 bad_empty.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 3 bad_empty.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 4 bad_huge.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 3 bad_huge.svg "Empty or too large SVG canvas")
fails(net.fxarena.openfx.ReadSVG 4 missing.svg "Cannot load frame|No filename")
fails(fr.inria.openfx.ReadMisc 1 bad_corrupt.bmp ".")
fails(fr.inria.openfx.ReadMisc 1 missing.bmp "Cannot load frame|No filename")
if(READPDF)
  fails(fr.inria.openfx.ReadPDF 1 missing.pdf "Cannot load frame|No filename")
endif()

# a debug build's property validation (Support's ofxsLog.cpp) writes an "ERROR : " line in the
# log of each run for a host property that is missing or wrong
file(GLOB _logs "${WORK}/*ofxTestLog.txt")
foreach(_l IN LISTS _logs)
  file(STRINGS "${_l}" _errors REGEX "ERROR : ")
  # every run here fails on purpose, and Support's mainEntry logs the exception that makes the
  # action fail as "ERROR : Caught ..." (an OFX::Exception::Suite, or ImageMagick's exception
  # for ReadMisc); the other ERROR lines are still checked
  list(FILTER _errors EXCLUDE REGEX "ERROR : Caught ")
  if(_errors)
    list(JOIN _errors "\n" _errors)
    message("FAIL ${_l}:\n${_errors}")
    list(APPEND _failed "${_l}")
  endif()
endforeach()
message("${_ok} passed, ${_n_run} run")
if(_failed)
  message(FATAL_ERROR "malformed reader test failed: ${_failed}")
endif()
