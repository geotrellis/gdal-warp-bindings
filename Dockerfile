FROM ubuntu:18.04
LABEL maintainer="James McClain <james.mcclain@gmail.com>"

RUN apt-get update -q && \
    apt-get install build-essential libboost-dev libgdal-dev openjdk-11-jdk wget -y && \
    apt-get autoremove && \
    apt-get autoclean && \
    apt-get clean

RUN wget 'https://download.osgeo.org/geotiff/samples/usgs/c41078a1.tif' -k -O /tmp/c41078a1.tif

WORKDIR /usr/local/src

# docker build -f Dockerfile -t jamesmcclain/gdal-build-environment:1 .
