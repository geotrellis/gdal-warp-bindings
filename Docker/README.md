# Dockerfile.crossbuild #

The file [`Dockerfile.crossbuild`](Dockerfile.crossbuild) is used to generate the Docker image `jamesmcclain/crossbuild` which contains the cross-compilers needed to generate Macintosh and Windows shared libraries.

This image is based heavily on Manfred Touron's [`multiarch/crossbuild`](https://github.com/multiarch/crossbuild) image (in fact, it is just that image with non-x86 compilers removed, some unneeded packages removed, and based on Ubuntu 18.04 instead of Debian Jessie).

An important legal note from that project's [README.md](https://github.com/multiarch/crossbuild/blob/3d8c2ea811534cc6f4d82f54892f5494681e2591/README.md) concerning Apple's OSX SDK is reproduced below:

## Legal note ##

OSX/Darwin/Apple builds: 
**[Please ensure you have read and understood the Xcode license
   terms before continuing.](https://www.apple.com/legal/sla/docs/xcode.pdf)**

# Dockerfile.environment #

The file [`Dockerfile.environment`](Dockerfile.environment) is used to generate the Docker image `jamesmcclain/gdal-build-environment` which is derived from `jamesmcclain/crossbuild`.  The former adds Macintosh and Windows versions of OpenJDK (which provide necessary header files) as well as binary version of GDAL for Macintosh and Windows (which need to be linked-against).

# License #

The Docker files in this directory are [MIT Licensed](LICENSE).
