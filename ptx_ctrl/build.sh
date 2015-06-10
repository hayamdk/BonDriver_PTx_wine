#!/bin/sh

gcc -m32 -O2 -Wall -fPIC -fshort-wchar -c ptx_ctrl.c
winegcc -m32 -shared -lm ptx_ctrl.spec ptx_ctrl.o -o ptx_ctrl.dll.so
mv ptx_ctrl.dll.so ptx_ctrl.dll
