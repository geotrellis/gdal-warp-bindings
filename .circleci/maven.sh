#!/bin/bash

if [ ! -z "${GPG_KEY}" ]
then
  mkdir -p ~/.m2
  cp /workdir/.circleci/settings.xml ~/.m2/

  echo "${GPG_KEY}" | base64 -d > signing_key.asc
  gpg --batch --passphrase "${GPG_PASSPHRASE}" --import signing_key.asc

  mvn gpg:sign-and-deploy-file -e \
    -Dgpg.passphrase="${GPG_PASSPHRASE}" \
    -DgroupId="com.azavea.geotrellis" \
    -DartifactId="gdal-warp-bindings" \
    -Dversion="33.${CIRCLE_SHA1:0:7}-SNAPSHOT" \
    -Dpackaging="jar" \
    -Dfile="/workdir/gdalwarp.jar" \
    -DrepositoryId="sonatype_snapshots" \
    -Durl="https://oss.sonatype.org/content/repositories/snapshots"
fi
