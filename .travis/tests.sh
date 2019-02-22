#!/bin/bash

docker run -it --rm \
      -v $(pwd):/usr/local/src/gdal-warp-bindings \
      -e CC=gcc -e CXX=g++ \
      jamesmcclain/gdal-build-environment:0 \
      make -C gdal-warp-bindings/src tests
