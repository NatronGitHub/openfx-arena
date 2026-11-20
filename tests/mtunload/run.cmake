# Render a few plug-ins with several threads, then unload the binary before exit, with mt_unload
# (mt_unload.cpp): each run must succeed and exit with status 0, without crashing, even at exit.
#
#   cmake -DMT_UNLOAD=<mt_unload> -DPLUGIN_PATH=<dir with Arena.ofx.bundle> -DSAMPLES=<dir>
#         -DOCIO=<config.ocio> -DWORK=<dir> [-DCACHE=<dir>] [-DTHREADS=4] [-DREPEAT=1] [-DREADPDF=ON]
#         [-DMAGICK_OPENMP=ON] -P run.cmake
#
# openfx-gmic crashed after a successful render when the host unloaded its binary: worker threads
# of a runtime that it links were still running, or were torn down by a static destructor of the
# unloaded binary. The plug-ins below start such threads: the Magick ones run ImageMagick on
# OFX::MultiThread::getNumCPUs() OpenMP threads (with openmp=1; their default is one thread), the
# Text, SVG and PDF ones use pango, cairo, librsvg, poppler and glib. Each one gets its own process,
# in hostDemo's default load mode (the binary is unloaded after the scan, loaded again for the
# render, and unloaded before exit). OMP_NUM_THREADS is THREADS, so that the OpenMP runtime has
# that many threads even on a machine with fewer CPUs. SAMPLES has the files of tests/hostdemo.
# READPDF=OFF: the bundle has no ReadPDF (OFX_LICENSE=COMMERCIAL).
# MAGICK_OPENMP=OFF: ImageMagick has no OpenMP (e.g. MacPorts' default variant), so the Magick
# plug-ins start no thread: the unload is still tested, but not the thread check.
cmake_minimum_required(VERSION 3.21...3.31) # as CMakeLists.txt
foreach(_v MT_UNLOAD PLUGIN_PATH SAMPLES OCIO WORK)
  if(NOT DEFINED ${_v})
    message(FATAL_ERROR "run.cmake: ${_v} is not set")
  endif()
  get_filename_component(${_v} "${${_v}}" ABSOLUTE)
endforeach()
if(NOT DEFINED CACHE)
  set(CACHE "${WORK}-cache")
endif()
get_filename_component(CACHE "${CACHE}" ABSOLUTE)
if(NOT DEFINED THREADS)
  set(THREADS 4)
endif()
if(NOT DEFINED REPEAT)
  set(REPEAT 1)
endif()
if(NOT DEFINED READPDF)
  set(READPDF ON)
endif()
if(NOT DEFINED MAGICK_OPENMP)
  set(MAGICK_OPENMP ON)
endif()

macro(native _out _path)
  if(CMAKE_HOST_WIN32)
    file(TO_NATIVE_PATH "${_path}" ${_out})
  else()
    set(${_out} "${_path}")
  endif()
endmacro()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(MAKE_DIRECTORY "${CACHE}")
native(_n "${PLUGIN_PATH}")
set(ENV{OFX_PLUGIN_PATH} "${_n}")
native(_n "${OCIO}")
set(ENV{OCIO} "${_n}")
native(_n "${WORK}")
set(ENV{HOME} "${_n}")
set(ENV{USERPROFILE} "${_n}")
# fontconfig's cache is kept between runs (as tests/hostdemo/run.cmake)
native(_n "${CACHE}")
set(ENV{XDG_CACHE_HOME} "${_n}")
if(CMAKE_HOST_WIN32)
  set(ENV{LOCALAPPDATA} "${_n}")
endif()
# hostDemo's default load mode, whatever the caller's environment says
unset(ENV{OFX_HOSTDEMO_KEEP_LOADED})
set(ENV{OMP_NUM_THREADS} "${THREADS}")
# keep the idle OpenMP pool threads spinning inside the runtime after the render, instead of
# blocking in a kernel wait after a short spin (libgomp's default spin count, libomp's 200 ms
# KMP_BLOCKTIME): a blocked thread is killed at exit without running any more code, so the test
# would pass even if the runtime were unmapped under it. A spinning thread crashes, during
# mt_unload's wait, every time the unload takes its runtime with it.
set(ENV{OMP_WAIT_POLICY} ACTIVE)
set(ENV{GOMP_SPINCOUNT} INFINITY) # libgomp (Linux, MinGW)
set(ENV{KMP_BLOCKTIME} infinite)  # libomp (macOS)

set(_ok 0)
set(_failed "")
# the minimum number of threads in the process after the render (mt_unload --min-threads): 2 for
# the plug-ins that leave OpenMP pool threads behind, so that the test fails instead of silently
# testing nothing if ImageMagick has no OpenMP, or a parameter stops enabling it
set(_min 1)
# check(<identifier> <major> <context> [name=value...]); @SAMPLES@ in a value is SAMPLES
macro(check _id _major _ctx)
  native(_ns "${SAMPLES}")
  set(_args "")
  foreach(_a ${ARGN})
    string(REPLACE "@SAMPLES@" "${_ns}" _a "${_a}")
    list(APPEND _args "${_a}")
  endforeach()
  foreach(_i RANGE 1 ${REPEAT})
    execute_process(
      COMMAND "${MT_UNLOAD}" --threads ${THREADS} --min-threads ${_min} ${_id} ${_major} ${_ctx} ${_args}
      WORKING_DIRECTORY "${WORK}"
      RESULT_VARIABLE _r OUTPUT_VARIABLE _log ERROR_VARIABLE _log
      TIMEOUT 300)
    if("${_r}" STREQUAL "0")
      # mt_unload's report: the threads, and the images still loaded after the unload
      string(STRIP "${_log}" _log)
      message("ok   ${_log}")
      math(EXPR _ok "${_ok} + 1")
    else()
      # a crash gives a signal name ("Segmentation fault"), or an NTSTATUS on Windows
      message("FAIL ${_id} v${_major}: ${_r}\n${_log}")
      list(APPEND _failed "${_id}v${_major}")
    endif()
  endforeach()
endmacro()

# the Magick plug-ins with OpenMP: MagickPluginHelper (Morphology, Swirl) and their own render()
if(MAGICK_OPENMP)
  set(_min 2)
else()
  message("note: ImageMagick has no OpenMP: the Magick plug-ins start no OpenMP thread, so their thread check is skipped (only the unload is tested)")
endif()
check(net.fxarena.openfx.Charcoal 3 filter openmp=1)
check(net.fxarena.openfx.Sketch 3 filter openmp=1)
check(net.fxarena.openfx.Morphology 2 filter openmp=1)
check(net.fxarena.openfx.Swirl 3 filter openmp=1)
check(net.fxarena.openfx.Implode 3 filter openmp=1)
check(net.fxarena.openfx.Arc 5 filter openmp=1)
check(net.fxarena.openfx.Polar 5 filter openmp=1)
set(_min 1)
# infinite RoD with the default size: give them a size (as tests/hostdemo/arena.txt). They have
# openmp=1, but leave no pool thread behind (1 thread after the render on macOS), so no thread
# check
check(net.fxarena.openfx.Texture 3 general width=320 height=240 openmp=1)
check(net.fxarena.openfx.Text 5 general width=320 height=240 openmp=1)
# TextOFX (pango, cairo, fontconfig), and the readers that use glib-based libraries or
# ImageMagick
check(net.fxarena.openfx.Text 7 filter)
check(net.fxarena.openfx.ReadSVG 4 general filename=@SAMPLES@/sample.svg)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 general filename=@SAMPLES@/sample.pdf)
endif()
check(net.fxarena.openfx.ReadPSD 2 general filename=@SAMPLES@/sample.psd)

list(LENGTH _failed _nf)
message("${_ok} multi-threaded renders (${THREADS} threads) followed by an unload: ${_nf} failed")
if(_failed)
  message(FATAL_ERROR "mt_unload test failed: ${_failed}")
endif()
