#!/bin/bash

dtrace -G -s tick-dtrace.d -o tick-dtrace.o
dtrace -h -s tick-dtrace.d -o tick-dtrace.h
gcc -c tick-main.c
gcc -o tick tick-main.o tick-dtrace.o

readelf -n ./tick
