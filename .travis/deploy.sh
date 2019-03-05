#!/bin/bash

docker run -it --rm \
      -v $(pwd):/tmp \
      -e COMMIT -e CI_DEPLOY_USERNAME -e CI_DEPLOY_PASSWORD \
      maven:3 \
      /tmp/.travis/maven.sh
