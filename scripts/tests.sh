#!/bin/bash

ln -s /tmp/c41078a1.tif src/experiments/data/c41078a1.tif # sic

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e CFLAGS="-Wall -Wno-sign-compare -Werror -O0 -ggdb3 -DSO_FINI -D_GNU_SOURCE" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      jamesmcclain/gdal-build-environment:6 make -j4 -C src tests || exit -1

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CC=gcc -e CXX=g++ \
      -e JAVA_HOME="/usr/lib/jvm/java-8-openjdk-amd64" \
      jamesmcclain/gdal-build-environment:6 make -j4 -C src/experiments/thread pattern oversubscribe || exit -1

rm -f $(find | grep '\.o$')
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
      -e LDFLAGS="-mmacosx-version-min=10.9 -L/macintosh/gdal/3.1.2/lib -lgdal -lstdc++ -lpthread" \
      jamesmcclain/gdal-build-environment:6 make -j4 -C src libgdalwarp_bindings.dylib || exit -1

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CROSS_TRIPLE=x86_64-w64-mingw32 \
      -e OS=win32 \
      -e CFLAGS="-Wall -Werror -Os -g" \
      -e JAVA_HOME="/windows/jdk8u202-b08" \
      -e GDALCFLAGS="-I/usr/local/include" \
      -e BOOST_ROOT="/usr/local/include/boost_1_69_0" \
      -e LDFLAGS="-L/windows/gdal/lib -lgdal_i -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic -lws2_32" \
      jamesmcclain/gdal-build-environment:6 make -j4 -C src gdalwarp_bindings.dll || exit -1

rm -f src/main/resources/*.so
mv src/libgdalwarp_bindings.so src/main/resources/resources/
mv src/libgdalwarp_bindings.dylib src/main/resources/resources/
mv src/gdalwarp_bindings.dll src/main/resources/resources/
mvn package

rm -f src/com_azavea_gdal_GDALWarp.h
rm -f src/experiments/thread/oversubscribe src/experiments/thread/pattern
