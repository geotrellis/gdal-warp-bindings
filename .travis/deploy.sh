#!/bin/bash

docker run -it --rm \
      -v $(pwd):/workdir \
      -e COMMIT -e CI_DEPLOY_USERNAME -e CI_DEPLOY_PASSWORD \
      maven:3 \
      /workdir/.travis/maven.sh
