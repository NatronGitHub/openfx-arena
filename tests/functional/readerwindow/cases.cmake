# The functional cases of reader_window (tests/common/reader_window.cmake, which defines check()):
# every reader on a render window smaller than the image, partly or entirely outside it, at
# scale 1 and 0.5, in both paths of GenericReader::render() (#105, PR #106; #108, PR #113).
# check(<identifier> <major> <sample> [name=value...] [reader_window options])
# filePremult: 1 premultiplied, 2 unpremultiplied (the readers are RGBA only, so the menu is
# Opaque, PreMultiplied, UnPreMultiplied). The default of ReadSVG is premultiplied (decode()
# into the output image), the others' is unpremultiplied (decode() into a temporary image).
check(net.fxarena.openfx.ReadSVG 4 sample.svg)
check(net.fxarena.openfx.ReadSVG 4 sample.svg filePremult=2)
# dpi: a canvas scaled by a DPI other than the default, so the windows are at other offsets
check(net.fxarena.openfx.ReadSVG 4 sample.svg dpi=150)
# the transform: the whole transformed canvas is rendered, whatever the window
check(net.fxarena.openfx.ReadSVG 4 sample.svg transformRotate=20)
check(net.fxarena.openfx.ReadSVG 3 sample.svg)
check(net.fxarena.openfx.ReadSVG 3 sample.svg filePremult=2)
check(net.fxarena.openfx.ReadSVG 3 sample.svg dpi=150)
# the sample_xy files differ along x and y (the other samples are a vertical gradient)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 sample.pdf)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf filePremult=1)
  # a page scaled by a DPI other than the default (150)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf dpi=100)
endif()
check(fr.inria.openfx.ReadKrita 2 sample.kra)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra filePremult=1)
check(fr.inria.openfx.OpenRaster 2 sample.ora)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora filePremult=1)
# a layer without x and y (optional in stack.xml, 0 by default, #119): at the top-left corner
check(fr.inria.openfx.OpenRaster 2 sample_noxy.ora --placement "${SAMPLES}/sample_xy.ora" 0 12)
# ReadPSD and ReadMisc already copied just the window (#105): they must keep doing it
check(net.fxarena.openfx.ReadPSD 2 sample.psd)
check(fr.inria.openfx.ReadMisc 1 sample.bmp)
# at half scale (a 50% viewer, proxy mode): decode() gets a render scale of 1 and the window at
# full resolution, and GenericReader downscales it (#108); both paths of render(), as above
check(net.fxarena.openfx.ReadSVG 4 sample.svg --scale 0.5)
check(net.fxarena.openfx.ReadSVG 4 sample.svg filePremult=2 --scale 0.5)
check(net.fxarena.openfx.ReadSVG 3 sample.svg --scale 0.5)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf --scale 0.5)
  check(fr.inria.openfx.ReadPDF 1 sample_xy.pdf filePremult=1 --scale 0.5)
endif()
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra --scale 0.5)
check(fr.inria.openfx.ReadKrita 2 sample_xy.kra filePremult=1 --scale 0.5)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora --scale 0.5)
check(fr.inria.openfx.OpenRaster 2 sample_xy.ora filePremult=1 --scale 0.5)
check(net.fxarena.openfx.ReadPSD 2 sample.psd --scale 0.5)
check(fr.inria.openfx.ReadMisc 1 sample.bmp --scale 0.5)
