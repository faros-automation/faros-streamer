#!/bin/bash

# Example of how to use them

# -C 2 = DS field set to 2
# NB: Q va regolato in modo che il pck non sia mai > 1500 ... altrimenti il decoder crasha
./faros_polito_IMG_sender -w 320*240 -H 127.0.0.1 -P 2000 -J 15 -Q 40 -T 1 -C 2 # [ -G ] # grayscale  [-S 3] # skip 3 frames every 3+1
./faros_polito_IMG_receiver -w 320*240 -H 127.0.0.1 -P 2000 -J 15 -T 1

# How to run in order to use libturbojpeg
LD_LIBRARY_PATH=/opt/libjpeg-turbo/lib  ./faros_polito_IMG_sender -w 320*240 -p 5 -H 127.0.0.1 -P 2000 -J 6 -Q 40 -C 2

./faros_polito_IMG_sender -w 640*480 -H 127.0.0.1 -P 2000 -J 30 -Q 15 -T 1 -C 2
./faros_polito_IMG_receiver -w 640*480 -H 127.0.0.1 -P 2000 -J 30 -T 1

