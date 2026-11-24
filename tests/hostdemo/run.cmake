# Render every plug-in of a bundle once with openfx's hostDemo (the reference is SupportExt's
# tests/hostdemo/run.sh), and fail on any load, describe, instance or render error, on a
# crash, on a missing output frame, and on a plug-in that the list doesn't mention.
#
#   cmake -DHOSTDEMO=<hostDemo> -DVERIFY=<verify_plugin_loads> -DBINARY=<bundle>/Contents/<arch>/<name>.ofx
#         -DLIST=<list.txt> -DSAMPLES=<dir> -DWORK=<dir> [-DCACHE=<dir>] [-DOFX14=ON]
#         [-DKEEP_LOADED=ON] -P run.cmake
#
# OFX14=ON runs hostDemo with --ofx14, as an OFX 1.4 host (only the OFX 1.4 host properties
# and suites, no OfxSetHost, no StrChoice or Bytes params, one view), to check that every
# plug-in also works in a host that doesn't implement OFX 1.5 or the extensions.
#
# Each plug-in is rendered by its own hostDemo process, without the plug-in cache. By default,
# hostDemo's scan sends every plug-in of the binary kOfxActionLoad, kOfxActionDescribe and
# kOfxActionUnload, then unloads the binary, and the first use loads it again (OfxSetHost, Load,
# Describe, then the instance): every run checks that the plug-ins survive two load cycles of the
# binary in one process (openfx #174). KEEP_LOADED=ON sets OFX_HOSTDEMO_KEEP_LOADED=1 instead
# (Host::keepPluginsLoadedAfterScan(), as Natron 2.6): one load cycle, the binary stays loaded.
#
# WORK is wiped at each run; CACHE (default: WORK-cache) is not: it holds the caches that
# would otherwise go to the home directory, e.g. fontconfig's.
#
# Relative paths are relative to the current directory. The list has a line per plug-in:
# "<identifier> <major> <context> [hostDemo arguments]", or "skip <identifier> <major> <reason>",
# and may have more lines for the same plug-in (each is rendered, e.g. with edge-case values);
# "#" starts a comment; the arguments are split as a shell would, then @SAMPLES@ is replaced
# in each, so SAMPLES may contain spaces.
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt
foreach(_v HOSTDEMO VERIFY BINARY LIST SAMPLES WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "run.cmake: ${_v} is not set")
  endif()
  # in script mode, relative to the current directory
  get_filename_component(${_v} "${${_v}}" ABSOLUTE)
endforeach()
if(NOT DEFINED CACHE)
  set(CACHE "${WORK}-cache")
endif()
get_filename_component(CACHE "${CACHE}" ABSOLUTE)
set(_hostargs "")
if(OFX14)
  set(_hostargs --ofx14)
endif()
file(MAKE_DIRECTORY "${CACHE}")
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
  list(APPEND _entries "${_l}")
endforeach()

# native paths for hostDemo: backslashes on Windows (file(TO_NATIVE_PATH) would also escape
# the spaces elsewhere)
macro(native _out _path)
  if(CMAKE_HOST_WIN32)
    file(TO_NATIVE_PATH "${_path}" ${_out})
  else()
    set(${_out} "${_path}")
  endif()
endmacro()

# the bundle directory: OFX_PLUGIN_PATH is its parent
get_filename_component(_bundle "${BINARY}" DIRECTORY) # Contents/<arch>
get_filename_component(_bundle "${_bundle}" DIRECTORY) # Contents
get_filename_component(_bundle "${_bundle}" DIRECTORY) # <name>.ofx.bundle
get_filename_component(_pluginpath "${_bundle}" DIRECTORY)
native(_pluginpath "${_pluginpath}")
# the readers need an OpenColorIO config: a minimal one, next to this script
native(_ocio "${CMAKE_CURRENT_LIST_DIR}/config.ocio")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
# hostDemo's environment (it is run directly, so that a crash is reported as such): the
# bundle, the OCIO config, and HOME in the work directory, so that the plug-ins' files in
# the home directory (e.g. HaldCLUT's ~/.clut) stay in the build tree
native(_nwork "${WORK}")
set(ENV{OFX_PLUGIN_PATH} "${_pluginpath}")
set(ENV{OCIO} "${_ocio}")
set(ENV{HOME} "${_nwork}")
set(ENV{USERPROFILE} "${_nwork}")
# the load mode, whatever the caller's environment says: the default (unload after the scan)
# unless KEEP_LOADED
if(KEEP_LOADED)
  set(ENV{OFX_HOSTDEMO_KEEP_LOADED} 1)
else()
  unset(ENV{OFX_HOSTDEMO_KEEP_LOADED})
endif()
# ... but the caches in a directory that is kept, so that fontconfig doesn't rebuild its
# cache at every run (XDG_CACHE_HOME; LOCALAPPDATA on Windows)
native(_ncache "${CACHE}")
set(ENV{XDG_CACHE_HOME} "${_ncache}")
if(CMAKE_HOST_WIN32)
  set(ENV{LOCALAPPDATA} "${_ncache}")
endif()

set(_ok 0)
set(_skipped 0)
set(_failed "")
foreach(_p IN LISTS _plugins)
  string(REGEX REPLACE "^plugin\\[[0-9]+\\] : ([^ ]+) v([0-9]+)$" "\\1;\\2" _idv "${_p}")
  list(GET _idv 0 _id)
  list(GET _idv 1 _major)
  # every line of the plug-in is rendered (e.g. the defaults, then edge-case values): the
  # first one to <name><major>, the next ones to <name><major>_<n> (a skip line skips only
  # itself); the counts below are per line
  set(_matched "")
  string(REGEX REPLACE "([][+.*()^$?|\\\\{}])" "\\\\\\1" _rid "${_id}")
  foreach(_e IN LISTS _entries)
    if(_e MATCHES "^(skip )?${_rid} ${_major}( |$)")
      list(APPEND _matched "${_e}")
    endif()
  endforeach()
  if(NOT _matched)
    message("FAIL ${_id} v${_major}: not in ${LIST}")
    list(APPEND _failed "${_id}v${_major}")
    continue()
  endif()
  set(_n_entry 0)
  foreach(_entry IN LISTS _matched)
    math(EXPR _n_entry "${_n_entry} + 1")
    separate_arguments(_split UNIX_COMMAND "${_entry}")
    set(_args "")
    foreach(_a IN LISTS _split)
      string(REPLACE "@SAMPLES@" "${SAMPLES}" _a "${_a}")
      list(APPEND _args "${_a}")
    endforeach()
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
    if(_n_entry GREATER 1)
      string(APPEND _prefix "_${_n_entry}")
    endif()
    set(_what "${_id} v${_major} (${_ctx})")
    if(_n_entry GREATER 1)
      list(JOIN _args " " _joined)
      string(APPEND _what " ${_joined}")
    endif()
    native(_nprefix "${_prefix}")
    # each run its own debug log: the default ofxTestLog.txt is opened with "w" by each run
    set(ENV{OFX_PLUGIN_LOGFILE} "${_nprefix}.ofxTestLog.txt")
    execute_process(
      COMMAND "${HOSTDEMO}" --quiet --no-std-paths --no-cache -p "${_id}" -m "${_major}" -c "${_ctx}"
              --frames 0 -o "${_nprefix}" ${_hostargs} ${_args}
      WORKING_DIRECTORY "${WORK}"
      RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
      TIMEOUT 600)
    # hostDemo writes <prefix>.0.ppm, or <prefix>.0l.ppm and <prefix>.0r.ppm (multi-view);
    # with --ofx14 (one view), always <prefix>.0.ppm
    set(_frame FALSE)
    if(EXISTS "${_prefix}.0.ppm" AND NOT (OFX14 AND (EXISTS "${_prefix}.0l.ppm" OR EXISTS "${_prefix}.0r.ppm")))
      file(SIZE "${_prefix}.0.ppm" _s)
      if(_s GREATER 0)
        set(_frame TRUE)
      endif()
    elseif(NOT OFX14 AND EXISTS "${_prefix}.0l.ppm" AND EXISTS "${_prefix}.0r.ppm")
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
      message("ok   ${_what}")
      math(EXPR _ok "${_ok} + 1")
    else()
      if("${_r}" STREQUAL "0")
        set(_r "exit 0, but no output frame")
      endif()
      message("FAIL ${_what}: ${_r}\n${_log}")
      list(APPEND _failed "${_id}v${_major}")
    endif()
  endforeach()
endforeach()
# a debug build's property validation (Support's ofxsLog.cpp) writes an "ERROR : " line in the
# log of each run for a host property that is missing or wrong
file(GLOB _logs "${WORK}/*ofxTestLog.txt")
foreach(_l IN LISTS _logs)
  file(STRINGS "${_l}" _errors REGEX "ERROR : ")
  if(_errors)
    list(JOIN _errors "\n" _errors)
    message("FAIL ${_l}:\n${_errors}")
    list(APPEND _failed "${_l}")
  endif()
endforeach()
string(TIMESTAMP _end "%s")
math(EXPR _t "${_end} - ${_start}")
list(LENGTH _plugins _n)
list(LENGTH _failed _nf)
set(_mode "")
if(OFX14)
  string(APPEND _mode " (OFX 1.4 host)")
endif()
if(KEEP_LOADED)
  string(APPEND _mode " (kept loaded after the scan)")
else()
  string(APPEND _mode " (unloaded after the scan)")
endif()
message("${_n} plug-ins${_mode}, renders: ${_ok} ok, ${_skipped} skipped, ${_nf} failed (${_t} s)")
if(_n EQUAL 0 OR _failed)
  message(FATAL_ERROR "render test failed: ${_failed}")
endif()
