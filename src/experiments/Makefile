PORT ?= 8080
NAME ?= gdal-warp-httpd

.PHONY: httpd-start httpd-stop tests data thread/rawthread thread/wrapthread
.SILENT: tests

all: tests cleaner

httpd-start:
	docker run -dit --rm --name $(NAME) -p $(PORT):80 -v $(shell pwd):/usr/local/apache2/htdocs/:ro httpd:2.4

httpd-stop:
	docker stop $(NAME)

data/c41078a1.tif:
	wget 'https://download.osgeo.org/geotiff/samples/usgs/c41078a1.tif' -k -O $@

data/c41078a1-local.vrt: data/c41078a1.tif
	gdalbuildvrt $@ $^

data/c41078a1-local-local.vrt: data/c41078a1-local.vrt
	gdalbuildvrt $@ $^

data/c41078a1-local-remote.vrt: data/c41078a1-local.vrt
	$(MAKE) -C . httpd-start
	gdalbuildvrt $@ "/vsicurl/http://localhost:$(PORT)/$(<)"
	$(MAKE) -C . httpd-stop

data/c41078a1-remote.vrt: data/c41078a1.tif
	$(MAKE) -C . httpd-start
	gdalbuildvrt $@ "/vsicurl/http://localhost:$(PORT)/$(<)"
	$(MAKE) -C . httpd-stop

data/c41078a1-remote-local.vrt: data/c41078a1-remote.vrt
	gdalbuildvrt $@ $^

data/c41078a1-remote-remote.vrt: data/c41078a1-remote.vrt
	$(MAKE) -C . httpd-start
	gdalbuildvrt $@ "/vsicurl/http://localhost:$(PORT)/$(<)"
	$(MAKE) -C . httpd-stop

data: data/c41078a1.tif data/c41078a1-local.vrt data/c41078a1-local-local.vrt data/c41078a1-local-remote.vrt data/c41078a1-remote.vrt data/c41078a1-remote-local.vrt data/c41078a1-remote-remote.vrt

thread/rawthread:
	$(MAKE) -C thread rawthread

thread/wrapthread:
	$(MAKE) -C thread wrapthread

tests: data thread/rawthread thread/wrapthread
	$(MAKE) -C . httpd-start
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-local-local.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-local-remote.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-local.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-remote-local.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-remote-remote.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1-remote.vrt
	# LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/rawthread data/c41078a1.tif
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-local-local.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-local-remote.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-local.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-remote-local.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-remote-remote.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1-remote.vrt 12 1024
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(shell pwd)/.. thread/wrapthread data/c41078a1.tif 12 1024
	$(MAKE) -C . httpd-stop

clean:
	$(MAKE) -C thread clean

cleaner: clean
	rm -f data/*.vrt thread/rawthread
	$(MAKE) -C thread cleaner

cleanest: cleaner
	rm -f data/*.tif
	$(MAKE) -C thread cleanest
