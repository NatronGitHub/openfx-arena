# - Try to find PangoFt
# Once done, this will define
#
#  PangoFt_FOUND - system has Pango
#  PangoFt_INCLUDE_DIRS - the Pango include directories
#  PangoFt_LIBRARIES - link these to use Pango

include(LibFindMacros)

# Dependencies
libfind_package(PangoFt Pango)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(PangoFt_PKGCONF pangoft2)

# Include dir
find_path(PangoFt_INCLUDE_DIR
  NAMES pango/pangoft2.h
  HINTS ${PangoFt_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES pango-1.0
)

# Finally the library itself
find_library(PangoFt_LIBRARY
  NAMES pangoft2-1.0
  HINTS ${PangoFt_PKGCONF_LIBRARY_DIRS}
)

libfind_process(PangoFt)

