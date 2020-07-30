# Blackmagick RAW

Blackmagic RAW is a modern, high performance, professional RAW codec that is open, cross platform and free.

*This plugin require a proprietary library available during runtime, it does not violate the GPL since we don't link against it, so it should be legal to distribute them together.*

## Supported cameras:

 * Blackmagic Design Pocket Cinema Camera 4K
 * Blackmagic Design URSA Mini Pro G2
 * Blackmagic Design Pocket Cinema Camera 6K
 * Blackmagic URSA Broadcast
 * Canon EOS C300 Mark II captured by Blackmagic Video Assist 12G HDR
 * Panasonic EVA1 captured by Blackmagic Video Assist 12G HDR
 * Sigma fp captured by Blackmagic Video Assist 12G HDR

## Build

```
make CONFIG=release
```

Then copy ``BlackmagicRAW.ofx.bundle`` where you want it. Note that the plugin expects ``libBlackmagicRawAPI.so`` /``BlackmagicRawAPI.dll`` in ``BlackmagicRAW.ofx.bundle/Contents/Resources/BlackmagicRAW/``.