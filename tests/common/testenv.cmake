# The environment setup and the ERROR-log check that the cmake -P test drivers of this
# directory share (include()d by hostdemo.cmake, reader_window.cmake, reader_malformed.cmake,
# mt_unload.cmake and readsvg_dpi.cmake). Each driver still sets what differs: the caches
# (XDG_CACHE_HOME, LOCALAPPDATA), the load mode and the threads.

# native paths for the programs under test: backslashes on Windows (file(TO_NATIVE_PATH) would
# also escape the spaces elsewhere)
macro(native _out _path)
  if(CMAKE_HOST_WIN32)
    file(TO_NATIVE_PATH "${_path}" ${_out})
  else()
    set(${_out} "${_path}")
  endif()
endmacro()

# test_env(<work> <plugin path> <ocio config>): wipes and creates the work directory, and sets
# the environment of the programs under test (they are run directly, so that a crash is reported
# as such): OFX_PLUGIN_PATH, OCIO, and HOME in the work directory, so that the plug-ins' files in
# the home directory (e.g. HaldCLUT's ~/.clut) stay in the build tree
function(test_env _work _pluginpath _ocio)
  file(REMOVE_RECURSE "${_work}")
  file(MAKE_DIRECTORY "${_work}")
  native(_n "${_pluginpath}")
  set(ENV{OFX_PLUGIN_PATH} "${_n}")
  native(_n "${_ocio}")
  set(ENV{OCIO} "${_n}")
  native(_n "${_work}")
  set(ENV{HOME} "${_n}")
  set(ENV{USERPROFILE} "${_n}")
endfunction()

# check_error_logs(<work> <out var> [<regex of ERROR lines to ignore>]): a debug build's property
# validation (Support's ofxsLog.cpp) writes an "ERROR : " line in the log of each run
# (OFX_PLUGIN_LOGFILE, <work>/*ofxTestLog.txt) for a host property that is missing or wrong
# (#228). Prints each log that has one, and sets <out var> to the list of those logs.
function(check_error_logs _work _out)
  file(GLOB _logs "${_work}/*ofxTestLog.txt")
  set(_bad "")
  foreach(_l IN LISTS _logs)
    file(STRINGS "${_l}" _errors REGEX "ERROR : ")
    if(ARGC GREATER 2)
      list(FILTER _errors EXCLUDE REGEX "${ARGV2}")
    endif()
    if(_errors)
      list(JOIN _errors "\n" _errors)
      message("FAIL ${_l}:\n${_errors}")
      list(APPEND _bad "${_l}")
    endif()
  endforeach()
  set(${_out} "${_bad}" PARENT_SCOPE)
endfunction()
