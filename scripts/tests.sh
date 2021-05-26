#!/bin/bash

ln -s /tmp/c41078a1.tif src/experiments/data/c41078a1.tif # sic

# Linux AMD64
docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e CFLAGS="-Wall -Werror -Og -ggdb3 -DSO_FINI -D_GNU_SOURCE" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      quay.io/geotrellis/gdal-warp-bindings-environment:amd64-2 make -j4 -C src tests || exit -1
docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      quay.io/geotrellis/gdal-warp-bindings-environment:amd64-2 make -j4 -C src/experiments/thread pattern oversubscribe || exit -1
rm -f $(find | grep '\.o$')

# Linux ARM64
docker run -it --rm \
      -v $(pwd):/workdir \
      -e ARCH=arm64 \
      -e CC=aarch64-linux-gnu-gcc-8 -e CXX=aarch64-linux-gnu-g++-8 \
      -e CFLAGS="-Wall -Werror -Og -ggdb3 -DSO_FINI -D_GNU_SOURCE" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-arm64" \
      -w /workdir \
      quay.io/geotrellis/gdal-warp-bindings-environment:arm64-2 make -j4 -C src libgdalwarp_bindings-arm64.so || exit -1
rm -f $(find | grep '\.o$')

# MacOS AMD64
docker run -it --rm \
      -v $(pwd):/workdir \
      -e OSXCROSS_NO_INCLUDE_PATH_WARNINGS=1 \
      -e CROSS_TRIPLE="x86_64-apple-darwin" \
      -e OS=darwin -e SO=dylib \
      -e CC=cc -e CXX=c++ \
      -e CFLAGS="-Wall -Werror -O0 -ggdb3" \
      -e JAVA_HOME="/macintosh/jdk8u202-b08/Contents/Home" \
      -e GDALCFLAGS="-I/macintosh/gdal/3.1.2/include" \
      -e CXXFLAGS="-I/usr/osxcross/SDK/MacOSX10.10.sdk/usr/include/c++/v1"  \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e LDFLAGS="-mmacosx-version-min=10.9 -L/macintosh/gdal/3.1.2/lib -lgdal -lstdc++ -lpthread -Wl,-rpath,/usr/local/lib" \
      quay.io/geotrellis/gdal-warp-bindings-environment:amd64-2 make -j4 -C src libgdalwarp_bindings-amd64.dylib || exit -1
rm -f $(find | grep '\.o$')

# Windows AMD64
docker run -it --rm \
      -v $(pwd):/workdir \
      -e CROSS_TRIPLE=x86_64-w64-mingw32 \
      -e OS=win32 \
      -e CFLAGS="-Wall -Werror -Os -g" \
      -e JAVA_HOME="/windows/jdk8u202-b08" \
      -e GDALCFLAGS="-I/usr/local/include" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e LDFLAGS="-L/windows/gdal/lib -lgdal_i -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic -lws2_32" \
      quay.io/geotrellis/gdal-warp-bindings-environment:amd64-2 make -j4 -C src gdalwarp_bindings-amd64.dll || exit -1
rm -f $(find | grep '\.obj$')

rm -f src/main/resources/*.so
mv src/libgdalwarp_bindings*.so src/main/resources/resources/
mv src/libgdalwarp_bindings*.dylib src/main/resources/resources/
mv src/gdalwarp_bindings*.dll src/main/resources/resources/

rm -f $(find src | grep '\.\(o\|obj\|dylib\|dll\|so\|class\)$')
rm -f src/com_azavea_gdal_GDALWarp.h
rm -f src/experiments/thread/oversubscribe src/experiments/thread/pattern
