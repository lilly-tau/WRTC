#!/usr/bin/env bash

[[ -d build ]] && rm -rf build

mkdir build
if [[ "$1" = debug ]]; then
	g++ -g -std=c++17 -obuild/wrtc src/main.cxx
else
	g++ -std=c++17 -obuild/wrtc src/main.cxx
fi

if [[ "$1" = "test" ]]; then
	build/wrtc test/main.wrt build/test.bin
	hexdump -C build/test.bin
elif [[ "$1" = "debug" ]]; then
	gdb -ex "set args test/main.wrt build/test.bin" build/wrtc
	hexdump -C build/test.bin
fi
