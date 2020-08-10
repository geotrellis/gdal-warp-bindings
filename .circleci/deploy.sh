#!/bin/bash

if [ ! -z "${GPG_KEY}" ]
then
  mkdir -p ~/.m2
  cp /workdir/.circleci/settings.xml ~/.m2/

  gpg --keyserver keyserver.ubuntu.com --recv-keys 0x13E9AA1D8153E95E

  echo "${GPG_KEY}" | base64 -d > signing_key.asc
  gpg --batch --passphrase "${GPG_PASSPHRASE}" --import signing_key.asc

  mvn deploy
fi
