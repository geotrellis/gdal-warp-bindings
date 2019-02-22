FROM ubuntu:18.04
LABEL maintainer="James McClain <james.mcclain@gmail.com>"

RUN apt-get update -q && \
    apt-get install build-essential libboost-dev libgdal-dev openjdk-11-jdk wget -y && \
    apt-get autoremove && \
    apt-get autoclean && \
    apt-get clean

WORKDIR /usr/local/src
