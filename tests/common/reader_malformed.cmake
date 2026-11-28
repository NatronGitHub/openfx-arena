# Render the readers on malformed or missing files with openfx's hostDemo: each must fail cleanly,
# with an error status and the reader's message, not crash, hang, or render garbage (the cases,
# each with its issue, are in tests/regression/readermalformed).
#
#   cmake -DHOSTDEMO=<hostDemo> -DPLUGIN_PATH=<dir with Arena.ofx.bundle> -DSAMPLES=<dir>
#         -DOCIO=<config.ocio> -DWORK=<dir> [-DREADPDF=ON] [-DCASES=<files>] -P reader_malformed.cmake
#
# SAMPLES has the files of make_samples.cpp, including the bad_ files that the cases use.
# A failure is an exit status other than 0 (a crash is reported by execute_process() as a string,
# e.g. "Segmentation fault", and fails the test), with no output frame, within the timeout.
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt
foreach(_v HOSTDEMO PLUGIN_PATH SAMPLES OCIO WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "reader_malformed.cmake: ${_v} is not set")
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
# fails(<identifier> <major> <file> <regex of the expected message>)
macro(fails _id _major _file _regex)
  math(EXPR _n_run "${_n_run} + 1")
  set(_prefix "${WORK}/run${_n_run}")
  native(_nfile "${SAMPLES}/${_file}")
  native(_nprefix "${_prefix}")
  # each run its own debug log, so that an ERROR line points at the run that wrote it
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

# the cases: fails() calls, from the case list(s) in CASES (by default regression/)
if(NOT DEFINED CASES)
  set(CASES "${CMAKE_CURRENT_LIST_DIR}/../regression/readermalformed/cases.cmake")
endif()
foreach(_c IN LISTS CASES)
  get_filename_component(_c "${_c}" ABSOLUTE)
  include("${_c}")
endforeach()

# the ERROR lines of a debug build's property validation; but every run here fails on purpose,
# and Support's mainEntry logs the exception that makes the action fail as "ERROR : Caught ..."
# (an OFX::Exception::Suite, or ImageMagick's exception for ReadMisc): the other ERROR lines are
# still checked
check_error_logs("${WORK}" _bad "ERROR : Caught ")
list(APPEND _failed ${_bad})
message("${_ok} passed, ${_n_run} run")
if(_failed)
  message(FATAL_ERROR "malformed reader test failed: ${_failed}")
endif()
