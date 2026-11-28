# The cases of render_Arena_mt_unload (tests/common/mt_unload.cmake, which defines check() and
# _min, the minimum number of threads after the render).
# check(<identifier> <major> <context> [name=value...]); @SAMPLES@ in a value is SAMPLES
# the Magick plug-ins with OpenMP: MagickPluginHelper (Morphology, Swirl) and their own render()
if(MAGICK_OPENMP)
  set(_min 2)
else()
  message("note: ImageMagick has no OpenMP: the Magick plug-ins start no OpenMP thread, so their thread check is skipped (only the unload is tested)")
endif()
check(net.fxarena.openfx.Charcoal 3 filter)
check(net.fxarena.openfx.Sketch 3 filter)
check(net.fxarena.openfx.Morphology 2 filter)
check(net.fxarena.openfx.Swirl 3 filter)
check(net.fxarena.openfx.Implode 3 filter)
check(net.fxarena.openfx.Arc 5 filter)
check(net.fxarena.openfx.Polar 5 filter)
set(_min 1)
# infinite RoD with the default size: give them a size (as tests/functional/hostdemo/arena.txt). They leave
# no pool thread behind (1 thread after the render on macOS), so no thread check
check(net.fxarena.openfx.Texture 3 general width=320 height=240)
check(net.fxarena.openfx.Text 5 general width=320 height=240)
# TextOFX (pango, cairo, fontconfig), and the readers that use glib-based libraries or
# ImageMagick
check(net.fxarena.openfx.Text 7 filter)
check(net.fxarena.openfx.ReadSVG 4 general filename=@SAMPLES@/sample.svg)
if(READPDF)
  check(fr.inria.openfx.ReadPDF 1 general filename=@SAMPLES@/sample.pdf)
endif()
check(net.fxarena.openfx.ReadPSD 2 general filename=@SAMPLES@/sample.psd)
