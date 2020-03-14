#!/bin/bash
# MSYS2 wrapper for Travis CI Windows

# debug
env
uname -a
ls /
CWD=`pwd`

# make sure we got the proper packages installed, note that Travis may fail to download some packages
pacman -Syu --noconfirm openfx-arena-sdk || pacman -Syu --noconfirm openfx-arena-sdk || pacman -Syu --noconfirm openfx-arena-sdk

# do a regular makefile debug build
make MINGW=1
