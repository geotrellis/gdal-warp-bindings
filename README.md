[![CircleCI](https://circleci.com/gh/geotrellis/gdal-warp-bindings.svg?style=svg)](https://circleci.com/gh/geotrellis/gdal-warp-bindings) [![Maven Central](https://img.shields.io/maven-central/v/com.azavea.geotrellis/gdal-warp-bindings)](https://search.maven.org/artifact/com.azavea.geotrellis/gdal-warp-bindings) [![Join the chat at https://gitter.im/geotrellis/geotrellis](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/geotrellis/geotrellis?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# Intro #

Having multi-threaded access to raster data is an important prerequisite to constructing a high-performance GIS tile server.
If one wishes to use [GDAL's VRT functionality](https://www.gdal.org/gdal_vrttut.html) in this way, then it is necessary to ensure that no VRT dataset is simultaneously used by more than one thread, even for read-only operations.
The code in this repository attempts to address that issue by wrapping GDAL datasets in objects which abstract one or more identical datasets; the wrapped datasets can be safely used from multiple threads with less contention than would be the case with a simple mutex around one GDAL dataset.
APIs are provided for C and Java.

# APIs #

The C and Java APIs are very similar to each other.

`GDAL` must be initialized prior to use.
In these bindings, that is accomplished by calling the `init` function in C or the `GDALWarp.init` static method in Java.
It is not necessary to call the latter if the [default parameters used in the static initializer](https://github.com/geotrellis/gdal-warp-bindings/blob/master/src/main/java/com/azavea/gdal/GDALWarp.java#L72) are statisfactory, but it is okay to do so.

In both APIs, one requests a token (a `uint64_t` in C and a `long` in Java) corresponding to a URI, Warp Options pair by calling `get_token` in C or `GDALWarp.get_token` in Java.

That token is then passed as an argument to the various wrapper functions, much as one would pass the GDAL Dataset handle into the unwrapped GDAL functions.
For example, `get_overview_widths_heights` (respectively `GDALWarp.get_overview_widths_heights`) takes a C `uint64_t` (respectively a Java `long`) as its first argument.

Both APIs are simple, C-style interfaces.
If one is interested in seeing how these bindings can be elaborated into a fancier presentation, please the see [`geotrellis/geotrellis-contrib`](https://github.com/geotrellis/geotrellis-contrib/) repository.
There, you can see these bindings used within a Scala API.

## C ##

Please see [`src/bindings.h`](src/bindings.h) for the full C/C++ interface.

## Java ##

Please see [`src/main/java/com/azavea/gdal/GDALWarp.java`](src/main/java/com/azavea/gdal/GDALWarp.java) for the full Java interface

# SonaType Artifacts #

The binary artifacts are present on [SonaType](https://search.maven.org/artifact/com.azavea.geotrellis/gdal-warp-bindings).
Snaphost artifacts are present on the [SonaType Snapshots repo](https://oss.sonatype.org/content/repositories/snapshots).

This jar file contains Linux, Macintosh, and Windows shared libraries.

All native binaries are built for [AMD64](https://en.wikipedia.org/wiki/X86-64#AMD64); the Linux ones are linked against GDAL 2.4.3, The Macintosh ones are linked against [GDAL 2.4.2 from Homebrew](https://formulae.brew.sh/formula/gdal#default), and the Windows ones are linked against the [GDAL 2.4.3 MSVC 2015 build from GISinternals.com](http://www.gisinternals.com/release.php).

The class files in the jar were built with OpenJDK 8.

The jar file is reachable via Maven:
```
<dependency>
  <groupId>com.azavea.geotrellis</groupId>
  <artifactId>gdal-warp-bindings</artifactId>
  <version>x.x.x</version>
  <type>pom</type>
</dependency>
```

# Repository Structure #

The [`Docker`](Docker) directory contains files used to generate images used for continuous integration testing and deployment.
The [`src`](src) directory contains all of the source code for the library; the C/C++ files are in that directory.
[`src/main`](src/main) contains the code for the Java API.
[`src/unit_tests`](src/unit_tests) contains the C++ unit tests.

# Ports #

The repository contains [everything needed](Docker/Dockerfile.environment) to compile Linux, Macintosh, and Windows versions of the library in a Docker container.

## How to Build ##

All three can easily be compiled in the normal manner outside of a container if all dependencies are present.

### Linux ###

If a recent [GDAL](https://www.gdal.org/) and recent [JDK](https://openjdk.java.net/) are installed, the Linux version can be built by typing `make -C src` from the root directory of the cloned repository.
If one only wishes to use the C/C++ library, then type `make -C src libgdalwarp_bindings.so`.

### Macintosh ###

If a recent GDAL is installed (perhaps from Homebrew) and a recent JDK is installed, the Macintosh version can be built on a Macintosh by typing `OS=darwin SO=dylib make -C src` from the root directory of the cloned repository.
If one only wishes to use the C/C++ library, then type `OS=darwin SO=dylib make -C src libgdalwarp_bindings.dylib`.

### Windows ###

The Windows version has only been cross-built with [MinGW](http://www.mingw.org/wiki/InstallationHOWTOforMinGW) from within a Linux Docker container.
Please see the [test script](scripts/tests.sh) for more.
