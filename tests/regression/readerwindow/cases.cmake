# The regression cases of reader_window (tests/common/reader_window.cmake, which defines check()),
# the ctest test regression_reader_window: the Krita and OpenRaster placement bugs of #108 (PR #113).
# check(<identifier> <major> <sample> [name=value...] [reader_window options])
# a PNG smaller than the image size in the XML: anchored at the top-left corner, black around it
# (#108; it used to fail). --placement checks where it lands: sample_xy's image, 64x48, at
# (0,12) in the 80x60 RoD; and the OpenRaster layer at x=10, y=5 from the top-left corner, at
# (10,60-5-48=7)
check(fr.inria.openfx.ReadKrita 2 sample_big.kra --placement "${SAMPLES}/sample_xy.kra" 0 12)
check(fr.inria.openfx.OpenRaster 2 sample_big.ora --placement "${SAMPLES}/sample_xy.ora" 0 12)
check(fr.inria.openfx.OpenRaster 2 sample_offset.ora --placement "${SAMPLES}/sample_xy.ora" 10 7)
check(fr.inria.openfx.OpenRaster 2 sample_offset.ora filePremult=1 --placement "${SAMPLES}/sample_xy.ora" 10 7)
