.PHONY: httpd-start httpd-stop

httpd-start:
	docker run -dit --rm --name gdal-warp-httpd -p 8080:80 -v $(shell pwd)/data:/usr/local/apache2/htdocs/:ro httpd:2.4

httpd-stop:
	docker stop gdal-warp-httpd

data/c41078a1.tif:
	wget 'https://download.osgeo.org/geotiff/samples/usgs/c41078a1.tif' -k -O $@

data/c41078a1-local.vrt: data/c41078a1.tif
	gdalbuildvrt $@ $^

data/c41078a1-remote.vrt: data/c41078a1.tif
	make -C . httpd-start
	gdalbuildvrt $@ '/vsicurl/http://localhost:8080/c41078a1.tif'
	make -C . httpd-stop
