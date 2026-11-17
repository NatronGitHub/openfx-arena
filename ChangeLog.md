# openfx-arena ChangeLog

## Unreleased

 * Arena now targets ImageMagick 7 (static, Q32, HDRI, OpenMP), which Natron ships on every platform. ImageMagick 6 still builds and renders as before.
 * ``Charcoal``, ``Sketch``, ``Morphology``, ``Texture``: keep their ImageMagick 6 alpha with ImageMagick 7 (it was grey levels, noise, transparent or negative)
 * With ImageMagick 7, these render slightly differently from ImageMagick 6 (largest difference measured, in [0,1] units, and the share of pixels affected):
   * ``Edges``: the Sobel kernel loses some vertical edge segments (up to 0.94, 30-41% of the pixels, mean 0.014). The other kernels differ by at most 0.003.
   * ``Tile``: with some grids (e.g. 3 rows of 4), the last 2 columns are opaque black instead of transparent (alpha 1, 1% of the pixels).
   * ``Arc``: resampling differs at the edges of the distorted image (up to 0.04 on 1% of the pixels, mean 6e-5).
   * ``Polar``: resampling differs at the edges of the distorted image (up to 0.06 on 2% of the pixels, mean 1.5e-4).
   * ``Text`` (Magick, v5): glyph edges are antialiased slightly differently (up to 1.0 on glyph edges, 0.4-7% of the pixels, mean below 0.0013).
   * ``Polaroid``: the caption's glyph edges differ the same way (up to 0.35 on 3% of the pixels, mean 0.0006). The picture itself is unchanged.
   * ``Roll``: negative offsets now work. With ImageMagick 6 they went through an unsigned conversion and were ignored on arm64 (x=-123 didn't roll at all), or gave an undefined offset elsewhere (issue #73).
   * ``Charcoal``: colour differs where the source isn't opaque (up to 0.17, mean 0.0045).
   * ``Sketch``: the noise pattern differs, since ImageMagick 7 draws different random numbers (up to 1.0, mean 0.06-0.22). The mean colour is unchanged.
   * ``Morphology``: with a transparent source, the colour of Edge, EdgeIn, EdgeOut, TopHat, BottomHat and HitAndMiss differs (up to 0.38, mean up to 0.11), and Voronoi differs entirely. The other methods are unchanged.
   * ``Texture``: Plasma and Plasma Fractal are opaque instead of half transparent, and the Loops patterns differ (their values are far outside [0,1] with both versions). The noises, Stripes, Checkerboard and the gradients are unchanged.
   * ``ReadMisc``: some RGB PCX files that ImageMagick 6 read as fully transparent (e.g. Natron-Tests' TestImagePCX) are read as opaque.

## Natron-2.3.15

 * ``Text``: Added support for SRT subtitles
 * ``Morphology``: Added new plugin
 * ``AudioCurve``: Added new plugin
 * Improved Resolve/Fusion compatibility
 * ``Edges``: Fixed ImageMagick compatibility
 * ``ReadCDR/ReadSVG``: Fixed deprecated librsvg API
 * ``Texture``: Rename gradient radial to linear
 * Updated submodules
 * CMake support

## Natron-2.3.14

 * Updated OpenFX-IO

## Natron-2.3.13

 * ``ReadCDR/ReadSVG``: Premultiplied fix
 * Updated submodules

## Natron-2.3.12

 * ``Text``: Fixed font issues
 * Updated submodules
 * Various code fixes

## Natron-2.3.11

 * Font fixes
 * Updated submodules

## Natron-2.3.10

 * Updated submodules
