#!/bin/bash

echo "MAVEN.SH"
echo ${GPG_KEY}

if [ ! -z "${GPG_KEY}" ]
then
  mkdir -p ~/.m2
  cp /workdir/.circleci/settings.xml ~/.m2/

  echo "${GPG_KEY}" | base64 -d > signing_key.asc
  gpg --batch --passphrase "${GPG_PASSPHRASE}" --import signing_key.asc

  mvn deploy
fi
