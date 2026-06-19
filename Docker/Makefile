DOCKER_REGISTRY ?= daunnc

gdal-3-6-4:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.6.4 \
		--build-arg GDAL_MACOS=libgdal-3.6.4-hbff29b3_22.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-6-4-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.6.4-amd64 .

gdal-3-7-2:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.7.2 \
		--build-arg GDAL_MACOS=libgdal-3.7.2-h85f6614_6.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-7-2-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.7.2-amd64 .

gdal-3-8-2:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.8.2 \
		--build-arg GDAL_MACOS=libgdal-3.8.2-h89a805d_1.conda \
		--build-arg GDAL_WINDOWS=release-1916-x64-gdal-3-8-1-mapserver-8-0-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.8.2-amd64 .

gdal-3-11-5:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.11.5 \
		--build-arg PROJ_VERSION=9.8.1 \
		--build-arg GDAL_MACOS=libgdal-core-3.11.5-h24162b0_7.conda \
		--build-arg GDAL_WINDOWS=release-1944-x64-gdal-3-11-5-mapserver-8-4-1-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.11.5-amd64 .

gdal-3-12-4:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.12.4 \
		--build-arg PROJ_VERSION=9.8.1 \
		--build-arg GDAL_MACOS=libgdal-core-3.12.4-h2e1b34b_4.conda \
		--build-arg GDAL_WINDOWS=release-1944-x64-gdal-3-12-1-mapserver-8-6-0-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.12.4-amd64 .

# gisinternals does not publish a Windows build for the 3.13.x line; fall back to the 3.12.1 Windows libs.
gdal-3-13-1:
	docker buildx build \
		--platform linux/amd64 \
		--push \
		--build-arg GDAL_VERSION=3.13.1 \
		--build-arg PROJ_VERSION=9.8.1 \
		--build-arg GDAL_MACOS=libgdal-core-3.13.1-h2e1b34b_1.conda \
		--build-arg GDAL_WINDOWS=release-1944-x64-gdal-3-12-1-mapserver-8-6-0-libs.zip \
		-f Dockerfile.environment-amd64 \
		-t $(DOCKER_REGISTRY)/gdal-warp-bindings-environment:3.13.1-amd64 .