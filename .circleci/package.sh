#!/bin/bash

cp -f /tmp/workdir/* src/main/resources/resources/ && \
mvn package
