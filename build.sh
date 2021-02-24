#!/bin/sh
BUILD_TOOL="g++"
if which auc >/dev/null; then BUILD_TOOL="auc"; fi

$BUILD_TOOL test.cpp -std=c++2a -g -o test src/*.* -lpthread -Ilibs -Iinclude `sdl2-config --cflags --libs` -lGL
