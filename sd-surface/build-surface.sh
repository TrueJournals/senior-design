#!/bin/bash

# This script is responsible for building the sd-surface.cpp file into an
# executable.  This will only be run on the Pi, so we can perform whatever build
# optimizations we want.

g++ -o sd-surface surface.cpp SubJoystick.cpp -lSDL

echo "g++ status: $?"
