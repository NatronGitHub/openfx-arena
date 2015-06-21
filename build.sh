#!/bin/sh
CLEAN=1 MAGICK_LEGACY=1 MAGICK_MOD=1 sh deploy.sh || exit 1
CLEAN=1 PACKAGE=ArenaIO VERSION=1.0beta1 sh deploy.sh || exit 1

