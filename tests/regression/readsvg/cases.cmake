# The cases of readsvg_dpi (tests/common/readsvg_dpi.cmake, which defines render() and check()),
# on the documents of this directory (SRC): ReadSVG 3 applied the DPI twice to physical units, and
# version 4 fixes it while version 3 keeps it for old projects (#98).
# check <major> <file> <dpi> <width> <height> <tolerance in pixels>
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
