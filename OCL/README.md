# ArenaCL.ofx

Various OpenFX plugins using OpenCL.

## Compatibility

### Software

 * Natron *(2.x)*
 * Resolve *(15.x)*

*Other hosts may work, but have not been tested.*

### Hardware

 * Nvidia Kepler (or higher)
 * AMD TeraScale 2 (or higher)
 * Any Intel/AMD CPU using AMD APP SDK

## Build (on Linux)

```
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```