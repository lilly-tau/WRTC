#!/usr/bin/env bash

if [[ "$EUID" -eq 0 ]]; then
	echo "Do not run as root."
	exit 1
fi

sudo cp build/wrtc /usr/bin/wrtc
