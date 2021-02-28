#!/bin/sh
if which auc >/dev/null; then BUILD_TOOL="auc -v"; else BUILD_TOOL="g++"; fi

OUT="test"
SRCS="test.cpp src/*.* libs/glew/glew.c libs/crypto/*.c"
#FLAGS="-std=c++2a -g"
FLAGS="-g"
INCS="-Ilibs -Ilibs/crypto -Iinclude `sdl2-config --cflags`"
LIBS="`sdl2-config --libs` -lpthread -lGL"

$BUILD_TOOL -o $OUT $FLAGS $INCS $SRCS $LIBS
