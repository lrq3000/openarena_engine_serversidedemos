#!/bin/sh

export CC=i586-mingw32msvc-gcc
export WINDRES=windres.exe
export PLATFORM=mingw32
if [ !$ARCH ]
then
export ARCH=x86
fi
exec make $*
