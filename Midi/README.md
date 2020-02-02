# Midi.ofx

Experimental input/output MIDI plug-ins.

```
git clone https://github.com/NatronGitHub/openfx-arena
cd openfx-arena
git submodule update -i --recursive
cd Midi
git clone https://github.com/thestk/rtmidi RtMidi
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/OFX/Plugins ..
make
sudo make install
```
