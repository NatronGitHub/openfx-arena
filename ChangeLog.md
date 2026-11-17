# openfx-arena ChangeLog

## Unreleased

 * ``ReadPSD`` 2.8 (fixed in place, #74):
   * XCF, hosts other than Natron: the layer menu picks the right layer (it was off by one, and the top layer crashed), and Default merges the layers (it was the bottom layer)
   * XCF, every host including Natron: the output size is the canvas from the file, instead of the largest layer's size
   * PSD and XCF, every host, with ImageMagick 6: layers at negative offsets are placed correctly (ImageMagick 7 already did)
   * XCF: the merged Default skips hidden layers and uses the layers' blend modes. Layer opacity is still ignored (ImageMagick doesn't read it)

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
