#!/bin/bash

if [ ! -z "${GPG_KEY}" ]
then
  mkdir -p ~/.m2
  cp /workdir/.circleci/settings.xml ~/.m2/

  echo "${GPG_KEY}" | base64 -d > signing_key.asc
  gpg --batch --passphrase "${GPG_PASSPHRASE}" --import signing_key.asc

  if [ "${CIRCLE_BRANCH}" == "release" ]
  then
    mvn gpg:sign-and-deploy-file \
      -Dgpg.passphrase="${GPG_PASSPHRASE}" \
      -DrepositoryId="sonatype_releases" \
      -Durl="https://oss.sonatype.org/service/local/staging/deploy/maven2" \
      -Dfile="/workdir/gdalwarp.jar" \
      -Dsources="/workdir/gdalwarp-source.jar" \
      -Djavadoc="/workdir/gdalwarp-javadoc.jar" \
      -DpomFile=pom.xml
    mvn org.sonatype.plugins:nexus-staging-maven-plugin:1.6.8:rc-list \
      -DnexusUrl="https://oss.sonatype.org/" \
      -DserverId="sonatype_releases" 2>&1 > /tmp/moop.txt
    STAGING_REPO=$(cat /tmp/moop.txt | grep comazavea | tail -1 | cut -d' ' -f2)
    mvn org.sonatype.plugins:nexus-staging-maven-plugin:1.6.8:rc-close \
      -DnexusUrl="https://oss.sonatype.org/" \
      -DserverId="sonatype_releases" \
      -DstagingRepositoryId="${STAGING_REPO}"
    mvn org.sonatype.plugins:nexus-staging-maven-plugin:1.6.8:rc-release \
      -DnexusUrl="https://oss.sonatype.org/" \
      -DserverId="sonatype_releases" \
      -DstagingRepositoryId="${STAGING_REPO}"
  else
    mvn gpg:sign-and-deploy-file \
      -Dgpg.passphrase="${GPG_PASSPHRASE}" \
      -DgroupId="com.azavea.geotrellis" \
      -DartifactId="gdal-warp-bindings" \
      -Dversion="33.${CIRCLE_SHA1:0:7}-SNAPSHOT" \
      -Dpackaging="jar" \
      -Dfile="/workdir/gdalwarp.jar" \
      -DrepositoryId="sonatype_snapshots" \
      -Durl="https://oss.sonatype.org/content/repositories/snapshots"
  fi
fi
