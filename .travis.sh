#!/bin/bash
#
# Travis CI Windows/MSYS2 build script for openfx-arena
#

# debug
env
uname -a
ls /

CWD=`pwd`

# make sure we got the proper packages installed, note that Travis may fail to download some packages
pacman -Syu --noconfirm tree openfx-arena-sdk || pacman -Syu --noconfirm tree openfx-arena-sdk || pacman -Syu --noconfirm tree openfx-arena-sdk

# we need mt.exe
wget https://github.com/NatronGitHub/NatronGitHub.github.io/raw/master/files/bin/natron-windows-installer.zip
unzip natron-windows-installer.zip
cp natron-windows-installer/mingw64/bin/mt.exe /mingw64/bin/

# do a debug build
make MINGW=1 CONFIG=debug
make -C BlackmagicRAW CONFIG=debug

# do a release build
make MINGW=1 CONFIG=release
make -C BlackmagicRAW CONFIG=release

# package release build
cd $CWD/Bundle/*-release
REL=`pwd`
DLLS="libcdr-0.1.dll libcroco-0.6-3.dll libicuin61.dll libmad-0.dll libpoppler-93.dll libpoppler-glib-8.dll librevenge-0.0.dll librevenge-stream-0.0.dll libsox-3.dll libzip.dll"

cd Arena.ofx.bundle/Contents/Win64
echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > manifest
echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> manifest
echo "<assemblyIdentity name=\"Arena.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> manifest
for dll in $DLLS; do
    cp /mingw64/bin/$dll .
    echo "<file name=\"$dll\"></file>" >> manifest
done
echo "</assembly>" >> manifest
cat manifest

strip -s *.ofx *.dll
/mingw64/bin/mt.exe -nologo -manifest manifest -outputresource:"Arena.ofx;2"

cd $REL
zip -9 -r Arena.ofx.bundle-Win64.zip Arena.ofx.bundle
tree -lah
