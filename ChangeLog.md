# openfx-arena ChangeLog

## Unreleased

 * ``Text`` (TextOFX) 7.0 (#70):
   * The Center is in canonical coordinates, as the overlay shows it: the text is drawn at the Center when the pixel aspect ratio isn't 1 or the RoD doesn't start at (0,0) (e.g. an offset Source).
   * The text is drawn in canonical units, so it isn't stretched on a PAR-corrected viewer: the glyphs, rotation, skew, scale, circle, arc and stroke are right at any pixel aspect ratio. Auto size fits the text at any pixel aspect ratio. Canvas size is in pixels.
   * Rotate, Skew and Scale are applied in the overlay's order (scale, then skew, then rotate, about the Center), and Skew Order is honoured. The glyphs no longer overlap with a non-uniform scale or a skew.
   * When Transform is unchecked, the text is rotated, skewed and scaled about the Center (where the overlay is), instead of the center of the image for the rotation.
   * The Center is no longer 1 pixel too high, and the vertical scroll is scaled by the vertical render scale.
   * New: Line spacing, and font size and position relative to the frame (Size unit, Font size (%), Relative position, #19, #63).
 * ``Text`` (TextOFX) 6: deprecated (hidden from the menus, still used by old projects). It is now 6.17, with the parameters of 6.14 (Natron 2.5.0) and the bug fixes made since: Stroke dash length above 3, fontconfig leaks and an uninitialized RoD read, rendering a render window smaller than the RoD.
 * Arena now targets ImageMagick 7 (static, Q32, HDRI, OpenMP), which Natron 2.6 will ship on every platform. ImageMagick 6 still builds and renders as before.
 * ``Charcoal``, ``Sketch``, ``Morphology``, ``Texture``: with ImageMagick 7, alpha is now the same as with ImageMagick 6 (it was grey levels, noise, transparent or negative), except where ImageMagick 7 differs inherently: Morphology Voronoi, the Morphology intensity methods with more than 1 iteration, and Texture Plasma.
 * With ImageMagick 7, these render differently from ImageMagick 6 (largest difference measured, in [0,1] units, and the share of pixels affected):
   * ``Edges``: the Sobel kernel loses some vertical edge segments (up to 0.94, 30-41% of the pixels, mean 0.014). The other kernels differ by at most 0.003.
   * ``Tile``: with some grids (e.g. 3 rows of 4), the last 2 columns are opaque black instead of transparent (alpha 1, 1% of the pixels).
   * ``Arc``: resampling differs at the edges of the distorted image (up to 0.04 on 1% of the pixels, mean 6e-5).
   * ``Polar``: resampling differs at the edges of the distorted image (up to 0.06 on 2% of the pixels, mean 1.5e-4).
   * ``Text`` (Magick, v5): glyph edges are antialiased slightly differently (up to 1.0 on glyph edges, 0.4-7% of the pixels, mean below 0.0013).
   * ``Polaroid``: the caption's glyph edges differ the same way (up to 0.35 on 3% of the pixels, mean 0.0006). The picture itself is unchanged.
   * ``Roll``: negative offsets now work. With ImageMagick 6 they went through an unsigned conversion and were ignored on arm64 (x=-123 didn't roll at all), or gave an undefined offset elsewhere (issue #73).
   * ``Charcoal``: colour differs where the source isn't opaque (up to 1.0, mean 0.13 on a hard matte). Opaque sources are unchanged.
   * ``Sketch``: the noise pattern differs, since ImageMagick 7 draws different random numbers (up to 1.0, mean 0.078-0.35), and the result is darker: opaque areas by 0.06-0.08 on average, and where a premultiplied source is transparent, ImageMagick 7 gives black where ImageMagick 6 gave grey paper (0.5).
   * ``Morphology``: colour differs for Convolve and Correlate with shaped (Octagon, Disk) or zero-sum (Sobel, Laplacian) kernels, even on opaque sources (up to 0.96, mean up to 0.26; with more than 1 iteration, up to 1.0 with any kernel that isn't normalised, mean up to 0.40 on opaque sources and 0.72 with alpha); for Edge, EdgeIn, EdgeOut, TopHat, BottomHat and HitAndMiss on sources with alpha (up to 0.38, mean up to 0.14); for the intensity methods with more than 1 iteration (up to 1.0, mean up to 0.043); and entirely for Voronoi. Normalised kernels (Gaussian, Blur) and the other methods are unchanged.
   * ``Texture``: Plasma and Plasma Fractal are opaque instead of half transparent, and the Loops patterns differ (their values are far outside [0,1] with both versions). The noises, Stripes, Checkerboard and the gradients are unchanged.
   * ``ReadMisc``: some RGB PCX files that ImageMagick 6 read as fully transparent (e.g. Natron-Tests' TestImagePCX) are read as opaque.
 * ``Text`` (TextOFX), ``Bulge`` and ``Twirl`` (OpenCL) overlays: Shift and Control come from the host's keyboard modifiers (ofxNatron.h extension 13) when it passes them, so a modifier pressed before the overlay got the focus is taken into account. Other hosts behave as before.
 * ``ReadPSD`` 2.8 (fixed in place, #74):
   * XCF, hosts other than Natron: the layer menu picks the right layer (it was off by one, and the top layer crashed), and Default merges the layers (it was the bottom layer)
   * XCF, every host including Natron: the output size is the canvas from the file, instead of the largest layer's size
   * PSD and XCF, every host, with ImageMagick 6: layers at negative offsets are placed correctly (ImageMagick 7 already did)
   * XCF: the merged Default skips hidden layers and uses the layers' blend modes. Layer opacity is still ignored (ImageMagick doesn't read it)
 * ``ReadSVG`` 3.4: Transform parameters (translate, rotate, scale, skew, center), applied before rasterization so that the SVG stays sharp when it is scaled up. The result is clipped to the SVG canvas. The transform can be animated. The defaults are the identity, so existing projects render exactly as before (issue #25). The plug-in also has the Transform node's overlay, which Natron shows once its Read node forwards overlays to readers: Natron's Read node forwards overlays from Natron#1284 (RB-2.6); released Natron 2.5 doesn't. The parameters work without it.
 * ``ReadSVG``: debug builds no longer abort at render scales below 1 (proxy mode).

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
