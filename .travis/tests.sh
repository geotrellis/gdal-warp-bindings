#!/bin/bash

ln -s /tmp/c41078a1.tif src/experiments/data/c41078a1.tif
docker run -it --rm \
      -v $(pwd):/usr/local/src/gdal-warp-bindings \
      -e CC=gcc -e CXX=g++ -e JAVA_HOME \
      jamesmcclain/gdal-build-environment:2 \
      make -C gdal-warp-bindings/src tests
