#!/bin/bash

mkdir -p ~/.m2
cp /tmp/.travis/settings.xml ~/.m2/
mvn deploy:deploy-file \
  -DgroupId="com.azavea.gdal" \
  -DartifactId="gdal-warp-bindings" \
  -Dversion="33-$COMMIT" \
  -Dpackaging="jar" \
  -Dfile="/tmp/src/main/gdalwarp.jar" \
  -DrepositoryId="bintray" \
  -Durl="https://api.bintray.com/maven/azavea/geotrellis/gdal-warp-bindings/;publish=1"
