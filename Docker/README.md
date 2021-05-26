# Dockerfile.crossbuild #

The file [`Dockerfile.crossbuild`](Dockerfile.crossbuild) is used to generate the Docker image `quay.io/geotrellis/gdal-warp-bindings-crossbuild` which contains the cross-compilers needed to generate Macintosh and Windows shared libraries.

This image is based heavily on Manfred Touron's [`multiarch/crossbuild`](https://github.com/multiarch/crossbuild) image (in fact, it is just that image with non-x86 compilers removed, some unneeded packages removed, and based on Ubuntu 20.04 instead of Debian Jessie).

An important legal note from that project's [README.md](https://github.com/multiarch/crossbuild/blob/3d8c2ea811534cc6f4d82f54892f5494681e2591/README.md) concerning Apple's OSX SDK is reproduced below:

## Legal note ##

OSX/Darwin/Apple builds: 
**[Please ensure you have read and understood the Xcode license
   terms before continuing.](https://www.apple.com/legal/sla/docs/xcode.pdf)**

# Dockerfile.environment #

The file [`Dockerfile.environment-amd64`](Dockerfile.environment-amd64) is used to generate the Docker images found under `quay.io/geotrellis/gdal-warp-bindings-environment` that contain `amd64` compilers.  The Docker images with `amd64` images are derived from `quay.io/geotrellis/gdal-warp-bindings-crossbuild`.  The Dockerfile mentioned above adds Macintosh, Windows, and `amd64` Linux versions of OpenJDK and GDAL.

The file [`Dockerfile.environment-arm64`](Dockerfile.environment-arm64) is used to generate the Docker images found under `quay.io/geotrellis/gdal-warp-bindings-environment` that contain `arm64` compilers.  The `arm64` image is not derived directly from a Debian Buster base image and contains a `aarch64-gnu-linux` toolchain as well as OpenJDK and GDAL.

# License #

The Docker files in this directory are [MIT Licensed](LICENSE).
