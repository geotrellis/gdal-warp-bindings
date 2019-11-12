#!/bin/bash

docker run -it --rm \
      -v $(pwd):/workdir \
      -e CIRCLE_SHA1 \
      -e SONATYPE_USERNAME -e SONATYPE_PASSWORD \
      -e GPG_KEY -e GPG_KEY_ID -e GPG_PASSPHRASE \
      maven:3 \
      /workdir/.circleci/maven.sh
