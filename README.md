[![CircleCI](https://circleci.com/gh/geotrellis/gdal-warp-bindings.svg?style=svg)](https://circleci.com/gh/geotrellis/gdal-warp-bindings) [![Maven Central](https://img.shields.io/maven-central/v/com.azavea.geotrellis/gdal-warp-bindings)](https://search.maven.org/artifact/com.azavea.geotrellis/gdal-warp-bindings) [![Join the chat at https://gitter.im/geotrellis/geotrellis](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/geotrellis/geotrellis?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

# Intro #

Having multi-threaded access to raster data is an important prerequisite to constructing a high-performance GIS tile server.
If one wishes to use [GDAL's VRT functionality](https://www.gdal.org/gdal_vrttut.html) in this way, then it is necessary to ensure that no VRT dataset is simultaneously used by more than one thread, even for read-only operations.
The code in this repository attempts to address that issue by wrapping GDAL datasets in objects which abstract one or more identical datasets; the wrapped datasets can be safely used from multiple threads with less contention than would be the case with a simple mutex around one GDAL dataset.
APIs are provided for C and Java.

# Installation #

Beginning with **gdal-warp-bindings version** `3.7` we've adopted a new versioning scheme intended to simplify linking to native dependencies. Given a `{major}.{minor}.{patch}` version:
* `{major}.{minor}` - matches the **GDAL** version it is published for
  * e.g. gdal-warp-bindings of version `3.7.x` are suitable for GDAL versions `3.7.x` major = `3`, minor = `7`
* `{patch}` - this portion of the version corresponds to updates within **gdal-warp-bindings** and should remain compatible with **GDAL library** `{major}.{minor}` versions
  * This implies that there may be multiple **gdal-warp-bindings** releases for the same **GDAL library** version. All releases are compatible with the matching **GDAL library** `{major}.{minor}` version. Thus, higher patch versions are to be preferred.

These bindings require a GDAL installation on your machine with the appropriate matching version:

| GDAL Warp Bindings | OS                    |  GDAL  | Shared Library {so,dylib,dll} |
|--------------------|-----------------------|--------|-------------------------------|
|             3.10.0 | Linux, MacOS, Windows | 3.10.x | libgdal.so.36.3.10.3          |
|             3.9.1  | Linux, MacOS, Windows | 3.9.x  | libgdal.so.35.3.9.2           |
|             3.9.0  | Linux, MacOS, Windows | 3.9.x  | libgdal.so.35.3.9.0           |
|             3.8.0  | Linux, MacOS, Windows | 3.8.x  | libgdal.so.34.3.8.x           |
|             3.7.0  | Linux, MacOS, Windows | 3.7.x  | libgdal.so.33.3.7.x           |
|             3.6.4  | Linux, MacOS, Windows | 3.6.x  | libgdal.so.33.3.6.x           |
|             1.1.x  | Linux (AMD64)         | 3.1.2  | libgdal.so.27                 |
|             1.1.x  | Linux (ARM64)         | 2.4.0  | libgdal.so.20                 |
|             1.1.x  | MacOS (AMD64)         | 3.1.2  | libgdal.27.dylib              |
|             1.1.x  | Windows               | 3.0.4  | --                            |

## MacOS ##

For MacOS users, you must also ensure that the appropriate shared libary file (dylib) above is symlinked to `/usr/local/lib` from your GDAL installation with matching version.

For best results, we recommend explicitly installing the appropriate GDAL version via Conda and symlinking the appropriate lib. Here's an example for GDAL Warp Bindings 1.1.0 using GDAL 3.1.2:

```shell
conda create -n gdal-3.1.2
conda activate gdal-3.1.2
conda install -c conda-forge gdal==3.1.2
sudo ln -s $(conda info --base)/envs/gdal-3.1.2/lib/libgdal.27.dylib /usr/local/lib/libgdal.27.dylib
conda deactivate
```

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

The repository contains [everything needed](Docker/Dockerfile.environment) to compile Linux (AMD64 and ARM64), Macintosh, and Windows versions of the library in a Docker container.

## How to Build ##

All four can easily be compiled in the normal manner outside of a container if all dependencies are present.

### Linux ###

If a recent [GDAL](https://www.gdal.org/) and recent [JDK](https://openjdk.java.net/) are installed, the Linux version can be built by typing `make -C src` from the root directory of the cloned repository.
If one only wishes to use the C/C++ library, then type `make -C src libgdalwarp_bindings_amd64.so` or `ARCH=arm64 make -C src libgdalwarp_bindings_arm64.so`.

### Macintosh ###

If a recent GDAL is installed (perhaps from Homebrew) and a recent JDK is installed, the Macintosh version can be built on a Macintosh by typing `OS=darwin SO=dylib make -C src` from the root directory of the cloned repository.
If one only wishes to use the C/C++ library, then type `OS=darwin SO=dylib make -C src libgdalwarp_bindings_amd64.dylib`.

### Windows ###

The Windows version has only been cross-built with [MinGW](http://www.mingw.org/wiki/InstallationHOWTOforMinGW) from within a Linux Docker container.
Please see the [test script](scripts/tests.sh) for more.
