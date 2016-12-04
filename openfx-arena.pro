#
# Project file for use with qt-creator
#

OTHER_FILES += \
            .travis.yml \
            Makefile \
            Makefile.master \
            Makefile.io \
            LICENSE \
            COPYING \
            README.md \
            Bundle/Info.plist \
            Bundle/Makefile \
            Extra/Info.plist \
            Extra/Makefile \
            Extra/README.md \
            Magick/Info.plist \
            Magick/Makefile \
            Magick/README.md \
            OCL/Info.plist \
            OCL/Makefile \
            OCL/README.md \
            OCL/Twirl/net.fxarena.opencl.Twirl.cl \
            OCL/Twirl/Makefile \
            OCL/Twirl/Info.plist \
            OCL/Bulge/net.fxarena.opencl.Bulge.cl \
            OCL/Bulge/Makefile \
            OCL/Bulge/Info.plist \
            OCL/Ripple/net.fxarena.opencl.Ripple.cl \
            OCL/Ripple/Makefile \
            OCL/Ripple/Info.plist \
            OCL/Sharpen/net.fxarena.opencl.Sharpen.cl \
            OCL/Sharpen/Makefile \
            OCL/Sharpen/Info.plist \
            OCL/Cartoon/net.fxarena.opencl.Cartoon.cl \
            OCL/Cartoon/Makefile \
            OCL/Cartoon/Info.plist \
            OCL/Duotone/net.fxarena.opencl.Duotone.cl \
            OCL/Duotone/Makefile \
            OCL/Duotone/Info.plist \
            OCL/Edge/net.fxarena.opencl.Edge.cl \
            OCL/Edge/Makefile \
            OCL/Edge/Info.plist \
            OCL/CLFilter/Makefile \
            OCL/CLFilter/Info.plist \
            Magick/Makefile.Magick \
            Magick/Swirl/Makefile \
            Magick/Swirl/Info.plist \
            Magick/Wave/Makefile \
            Magick/Wave/Info.plist \
            Magick/Roll/Makefile \
            Magick/Roll/Info.plist \
            Magick/HaldCLUT/Info.plist \
            Magick/Makefile.Magick \
            Magick/HaldCLUT/net.fxarena.openfx.HaldCLUT.xml \
            Magick/HaldCLUT/Makefile \
            Magick/Arc/Makefile \
            Magick/Arc/Info.plist
INCLUDEPATH += . \
            SupportExt \
            OpenFX/Support/include \
            OpenFX/include \
            OpenFX-IO/IOSupport \
            OpenFX/Support/Plugins/include \
            OpenFX-IO/IOSupport/SequenceParsing \
            OpenFX/Support/Library \
            /usr/local/magick7/include/ImageMagick-7 \
            /usr/include \
            /usr/include/libxml2 \
            /usr/include/librevenge-0.0 \
            /usr/include/libcdr-0.1 \
            /usr/include/librsvg-2.0 \
            /usr/include/cairo \
            /usr/include/glib-2.0 \
            /usr/include/pango-1.0 \
            /usr/include/poppler/glib \
            OCL/OpenCL \
            OCL/OpenCL/CL \
            OCL \
            Magick
HEADERS += \
            SupportExt/ofxsCoords.h \
            SupportExt/ofxsCopier.h \
            SupportExt/ofxsFilter.h \
            SupportExt/ofxsFormatResolution.h \
            SupportExt/ofxsGenerator.h \
            SupportExt/ofxsImageBlenderMasked.h \
            SupportExt/ofxsLut.h \
            SupportExt/ofxsMacros.h \
            SupportExt/ofxsMaskMix.h \
            SupportExt/ofxsMatrix2D.h \
            SupportExt/ofxsMerging.h \
            SupportExt/ofxsMipmap.h \
            SupportExt/ofxsMultiPlane.h \
            SupportExt/ofxsOGLFontUtils.h \
            SupportExt/ofxsOGLTextRenderer.h \
            SupportExt/ofxsPixelProcessor.h \
            SupportExt/ofxsPositionInteract.h \
            SupportExt/ofxsRamp.h \
            SupportExt/ofxsRectangleInteract.h \
            SupportExt/ofxsShutter.h \
            SupportExt/ofxsTracking.h \
            SupportExt/ofxsTransform3x3.h \
            SupportExt/ofxsTransform3x3Processor.h \
            SupportExt/ofxsTransformInteract.h \
            OpenFX/include/ofxCore.h \
            OpenFX/include/ofxDialog.h \
            OpenFX/include/ofxImageEffect.h \
            OpenFX/include/ofxInteract.h \
            OpenFX/include/ofxKeySyms.h \
            OpenFX/include/ofxMemory.h \
            OpenFX/include/ofxMessage.h \
            OpenFX/include/ofxMultiThread.h \
            OpenFX/include/ofxNatron.h \
            OpenFX/include/ofxOld.h \
            OpenFX/include/ofxOpenGLRender.h \
            OpenFX/include/ofxParam.h \
            OpenFX/include/ofxParametricParam.h \
            OpenFX/include/ofxPixels.h \
            OpenFX/include/ofxProgress.h \
            OpenFX/include/ofxProperty.h \
            OpenFX/include/ofxSonyVegas.h \
            OpenFX/include/ofxTimeLine.h \
            OpenFX-IO/IOSupport/GenericOCIO.h \
            OpenFX-IO/IOSupport/GenericReader.h \
            OpenFX-IO/IOSupport/GenericWriter.h \
            OpenFX-IO/IOSupport/IOUtility.h \
            OpenFX/Support/include/ofxsCore.h \
            OpenFX/Support/include/ofxsHWNDInteract.h \
            OpenFX/Support/include/ofxsImageEffect.h \
            OpenFX/Support/include/ofxsInteract.h \
            OpenFX/Support/include/ofxsLog.h \
            OpenFX/Support/include/ofxsMemory.h \
            OpenFX/Support/include/ofxsMessage.h \
            OpenFX/Support/include/ofxsMultiThread.h \
            OpenFX/Support/include/ofxsParam.h \
            OpenFX/Support/Library/ofxsSupportPrivate.h \
            OpenFX-IO/IOSupport/SequenceParsing/SequenceParsing.h \
            OpenFX/Support/Plugins/Generator/randomGenerator.H \
            OpenFX/Support/Plugins/include/ofxsImageBlender.H \
            OpenFX/Support/Plugins/include/ofxsProcessing.H \
            OpenFX-IO/IOSupport/SequenceParsing/tinydir/tinydir.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Blob.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/ChannelMoments.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/CoderInfo.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Color.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Drawable.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Exception.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Functions.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Geometry.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Image.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Include.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Montage.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/Pixels.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/ResourceLimits.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/STL.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++/TypeMetric.h \
            /usr/local/magick7/include/ImageMagick-7/Magick++.h \
            OCL/ofxsTransformInteractCustom.h \
            OCL/OCLPlugin.h \
            Magick/MagickPlugin.h
SOURCES += \
            Extra/OpenRaster.cpp \
            ReadSVG/ReadSVG.cpp \
            Extra/ReadKrita.cpp \
            Extra/ReadCDR.cpp \
            Extra/TextFX.cpp \
            Extra/ReadPDF.cpp \
            Magick/Arc/Arc.cpp \
            Magick/Charcoal.cpp \
            Magick/Edges.cpp \
            Magick/Implode.cpp \
            Magick/Modulate.cpp \
            Magick/Oilpaint.cpp \
            Magick/Polar.cpp \
            Magick/Polaroid.cpp \
            Magick/Reflection.cpp \
            Magick/Roll.cpp \
            Magick/Sketch.cpp \
            Magick/Swirl.cpp \
            Magick/Text.cpp \
            Magick/Texture.cpp \
            Magick/Tile.cpp \
            Magick/Wave.cpp \
            Magick/ReadMisc.cpp \
            Magick/ReadPSD.cpp \
            OCL/ofxsTransformInteractCustom.cpp \
            OCL/OCLPlugin.cpp \
            OCL/Edge/Edge.cpp \
            OCL/Sharpen/Sharpen.cpp \
            OCL/Ripple/Ripple.cpp \
            OCL/Twirl/Twirl.cpp \
            OCL/TestCL/TestCL.cpp \
            OCL/HaldCLUT/HaldCLUT.cpp \
            OCL/Glow/Glow.cpp \
            OCL/Duotone/Duotone.cpp \
            OCL/Cartoon/Cartoon.cpp \
            OCL/Emboss/Emboss.cpp \
            OCL/Bulge/Bulge.cpp \
            OCL/FishEye/FishEye.cpp \
            OCL/Warp/Warp.cpp \
            OCL/Bokeh/Bokeh.cpp \
            OCL/CLFilter/CLFilter.cpp \
            Magick/MagickPlugin.cpp \
            Magick/Swirl/Swirl.cpp \
            Magick/Wave/Wave.cpp \
            Magick/Roll/Roll.cpp \
            Magick/HaldCLUT/HaldCLUT.cpp
