#!/bin/sh
if [ "$NOCLEAN" == "1" ]; then
  DOCLEAN=0
else
  DOCLEAN=1
fi
CLEAN=$DOCLEAN MAGICK_LEGACY=1 MAGICK_MOD=1 PACKAGE=Text VERSION=3.2 sh deploy.sh || exit 1
echo "Text.ofx DONE!!!"

