#!/bin/bash

set -e

mvn package && \
cd ./target && \
mkdir ./resources && \
cp /tmp/workdir/* ./resources/ && \
jar uf `ls ./ | grep jar` resources/* && \
cd ../
