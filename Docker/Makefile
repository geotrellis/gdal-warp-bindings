DOCKER_REGISTRY ?= quay.io/geotrellis

gdal-3-6-4:
	docker build \
		--build-arg GDAL_VERSION=3.6.4 \
		--build-arg GDAL_MACOS=libgdal-3.6.4-hbff29b3_22.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-6-4-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.6.4-amd64 .

gdal-3-7-2:
	docker build \
		--build-arg GDAL_VERSION=3.7.2 \
		--build-arg GDAL_MACOS=libgdal-3.7.2-h85f6614_6.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-7-2-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.7.2-amd64 .

gdal-3-8-2:
	docker build \
		--build-arg GDAL_VERSION=3.8.2 \
		--build-arg GDAL_MACOS=libgdal-3.8.2-h89a805d_1.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-8-1-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.8.2-amd64 .