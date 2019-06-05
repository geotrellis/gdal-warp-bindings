#!/bin/bash

ln -s /tmp/c41078a1.tif src/experiments/data/c41078a1.tif # sic

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e CFLAGS="-Wall -Werror -O0 -ggdb3 -DSO_FINI" \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      jamesmcclain/gdal-build-environment:3 make -j4 -C src tests || exit -1

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      jamesmcclain/gdal-build-environment:3 make -j4 -C src/experiments/thread pattern oversubscribe || exit -1

rm -f $(find | grep '\.o$')
docker run -it --rm \
      -v $(pwd):/workdir \
      -e OSXCROSS_NO_INCLUDE_PATH_WARNINGS \
      -e CROSS_TRIPLE="x86_64-apple-darwin" \
      -e OS=darwin -e SO=dylib \
      -e CC=cc -e CXX=c++ \
      -e JAVA_HOME="/macintosh/jdk8u202-b08/Contents/Home" \
      -e GDALCFLAGS="-I/macintosh/gdal/2.4.0_1/include" \
      -e CXXFLAGS="-I/usr/osxcross/SDK/MacOSX10.10.sdk/usr/include/c++/v1"  \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e LDFLAGS="-mmacosx-version-min=10.9 -L/macintosh/gdal/2.4.0_1/lib -lgdal -lstdc++ -lpthread" \
      jamesmcclain/gdal-build-environment:3 make -j4 -C src libgdalwarp_bindings.dylib || exit -1

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CROSS_TRIPLE=x86_64-w64-mingw32 \
      -e OS=win32 \
      -e CFLAGS="-Wall -O" \
      -e JAVA_HOME="/windows/jdk8u202-b08" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e LDFLAGS="-L/windows/gdal/lib -lgdal_i -lstdc++ -lpthread -lws2_32" \
      jamesmcclain/gdal-build-environment:3 make -j4 -C src gdalwarp_bindings.dll || exit -1

cp src/libgdalwarp_bindings.dylib src/main/java/resources/ || exit -1
cp src/gdalwarp_bindings.dll src/main/java/resources/ || exit -1
(cd src/main/java ; jar -cvf ../../../gdalwarp.jar com/azavea/gdal/*.class cz/adamh/utils/*.class resources/*) || exit -1

rm -f $(find | grep '\.\(o\|obj\|dylib\|dll\|so\|class\)$')
rm -f src/com_azavea_gdal_GDALWarp.h
rm -f src/experiments/thread/oversubscribe src/experiments/thread/pattern
