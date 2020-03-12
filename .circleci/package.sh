#!/bin/bash

rm -f src/main/resources/resources/.gitkeep && \
cp -f /tmp/workdir/* src/main/resources/resources/ && \
mvn package
