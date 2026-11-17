# Render every plug-in of a bundle once with openfx's hostDemo (the reference is SupportExt's
# tests/hostdemo/run.sh), and fail on any load, describe, instance or render error, on a
# crash, on a missing output frame, and on a plug-in that the list doesn't mention.
#
#   cmake -DHOSTDEMO=<hostDemo> -DVERIFY=<verify_plugin_loads> -DBINARY=<bundle>/Contents/<arch>/<name>.ofx
#         -DLIST=<list.txt> -DSAMPLES=<dir> -DWORK=<dir> -P run.cmake
#
# The list has one line per plug-in: "<identifier> <major> <context> [hostDemo arguments]",
# or "skip <identifier> <major> <reason>"; "#" starts a comment; @SAMPLES@ is replaced.
cmake_minimum_required(VERSION 3.16)
foreach(_v HOSTDEMO VERIFY BINARY LIST SAMPLES WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "run.cmake: ${_v} is not set")
  endif()
endforeach()
string(TIMESTAMP _start "%s")

# the bundle's plug-ins, as the load test sees them: "plugin[i] : <id> v<major>.<minor>"
execute_process(COMMAND "${VERIFY}" "${BINARY}" RESULT_VARIABLE _r OUTPUT_VARIABLE _out ERROR_VARIABLE _err)
if(NOT _r EQUAL 0)
  message(FATAL_ERROR "verify_plugin_loads failed (${_r}):\n${_out}${_err}")
endif()
string(REGEX MATCHALL "plugin\\[[0-9]+\\] : [^ \r\n]+ v[0-9]+" _plugins "${_out}")

# the list
file(STRINGS "${LIST}" _lines)
set(_entries "")
foreach(_l IN LISTS _lines)
  string(STRIP "${_l}" _l)
  if(_l STREQUAL "" OR _l MATCHES "^#")
    continue()
  endif()
  string(REPLACE "@SAMPLES@" "${SAMPLES}" _l "${_l}")
  list(APPEND _entries "${_l}")
endforeach()

# the bundle directory: OFX_PLUGIN_PATH is its parent
get_filename_component(_bundle "${BINARY}" DIRECTORY) # Contents/<arch>
get_filename_component(_bundle "${_bundle}" DIRECTORY) # Contents
get_filename_component(_bundle "${_bundle}" DIRECTORY) # <name>.ofx.bundle
get_filename_component(_pluginpath "${_bundle}" DIRECTORY)
file(TO_NATIVE_PATH "${_pluginpath}" _pluginpath)
# the readers need an OpenColorIO config: a minimal one, next to this script
file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/config.ocio" _ocio)
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")

set(_ok 0)
set(_skipped 0)
set(_failed "")
foreach(_p IN LISTS _plugins)
  string(REGEX REPLACE "^plugin\\[[0-9]+\\] : ([^ ]+) v([0-9]+)$" "\\1;\\2" _idv "${_p}")
  list(GET _idv 0 _id)
  list(GET _idv 1 _major)
  set(_entry "")
  foreach(_e IN LISTS _entries)
    if(_e MATCHES "^(skip )?${_id} ${_major}( |$)")
      set(_entry "${_e}")
      break()
    endif()
  endforeach()
  if(_entry STREQUAL "")
    message("FAIL ${_id} v${_major}: not in ${LIST}")
    list(APPEND _failed "${_id}v${_major}")
    continue()
  endif()
  separate_arguments(_args UNIX_COMMAND "${_entry}")
  if(_entry MATCHES "^skip ")
    string(REGEX REPLACE "^skip [^ ]+ [0-9]+ *" "" _reason "${_entry}")
    message("skip ${_id} v${_major}: ${_reason}")
    math(EXPR _skipped "${_skipped} + 1")
    continue()
  endif()
  list(GET _args 2 _ctx)
  list(REMOVE_AT _args 0 1 2)
  string(REGEX REPLACE "^.*\\." "" _name "${_id}")
  set(_prefix "${WORK}/${_name}${_major}")
  file(TO_NATIVE_PATH "${_prefix}" _nprefix)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "OFX_PLUGIN_PATH=${_pluginpath}" "OCIO=${_ocio}"
            "${HOSTDEMO}" --quiet --no-std-paths --no-cache -p "${_id}" -m "${_major}" -c "${_ctx}"
            --frames 0 -o "${_nprefix}" ${_args}
    WORKING_DIRECTORY "${WORK}"
    RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
    TIMEOUT 600)
  # hostDemo writes <prefix>.0.ppm, or <prefix>.0l.ppm and <prefix>.0r.ppm (multi-view)
  set(_frame FALSE)
  if(EXISTS "${_prefix}.0.ppm")
    file(SIZE "${_prefix}.0.ppm" _s)
    if(_s GREATER 0)
      set(_frame TRUE)
    endif()
  elseif(EXISTS "${_prefix}.0l.ppm" AND EXISTS "${_prefix}.0r.ppm")
    file(SIZE "${_prefix}.0l.ppm" _s)
    file(SIZE "${_prefix}.0r.ppm" _s2)
    if(_s GREATER 0 AND _s2 GREATER 0)
      set(_frame TRUE)
    endif()
  endif()
  file(GLOB _outputs "${_prefix}.*.ppm")
  if(_outputs)
    file(REMOVE ${_outputs}) # PAL SD PPM files are a few MB each
  endif()
  if("${_r}" STREQUAL "0" AND _frame)
    message("ok   ${_id} v${_major} (${_ctx})")
    math(EXPR _ok "${_ok} + 1")
  else()
    if("${_r}" STREQUAL "0")
      set(_r "exit 0, but no output frame")
    endif()
    message("FAIL ${_id} v${_major} (${_ctx}): ${_r}\n${_log}")
    list(APPEND _failed "${_id}v${_major}")
  endif()
endforeach()
string(TIMESTAMP _end "%s")
math(EXPR _t "${_end} - ${_start}")
list(LENGTH _plugins _n)
list(LENGTH _failed _nf)
message("${_n} plug-ins: ${_ok} rendered, ${_skipped} skipped, ${_nf} failed (${_t} s)")
if(_n EQUAL 0 OR _failed)
  message(FATAL_ERROR "render test failed: ${_failed}")
endif()
