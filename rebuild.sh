#!/bin/bash

rm -rf bin build
cmake -S . -B build
cmake --build build

cd bin
