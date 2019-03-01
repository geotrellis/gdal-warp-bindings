JAVA ?= java
JAVAC ?= javac
SO ?= so

.PHONY: tests clean cleaner cleanest

gdalwarp.jar: java/resources/libgdalwarp_bindings.$(SO) java/com/azavea/gdal/GDALWarp.class java/cz/adamh/utils/NativeUtils.class
	(cd java ; jar -cvf ../$@ com/azavea/gdal/*.class cz/adamh/utils/*.class resources/*)

../libgdalwarp_bindings.$(SO):
	make -C .. libgdalwarp_bindings.$(SO)

java/resources/libgdalwarp_bindings.$(SO): ../libgdalwarp_bindings.$(SO)
	cp -f $< $@

java/com/azavea/gdal/GDALWarp.class java/cz/adamh/utils/NativeUtils.class: java/com/azavea/gdal/GDALWarp.java java/cz/adamh/utils/NativeUtils.java
	$(JAVAC) -h .. -cp java $<

GDALWarpThreadTest.class: GDALWarpThreadTest.java gdalwarp.jar
	$(JAVAC) -cp gdalwarp.jar $<

# MIT-Licensed code (that license is compatible with Apache 2.0)
java/cz/adamh/utils/NativeUtils.java:
	wget -k "https://raw.githubusercontent.com/adamheinrich/native-utils/e6a39489662846a77504634b6fafa4995ede3b1d/src/main/$@" -O $@

../experiments/data/c41078a1.tif:
	make -C ../experiments data/c41078a1.tif

tests: gdalwarp.jar GDALWarpThreadTest.class ../experiments/data/c41078a1.tif
	$(JAVA) -cp gdalwarp.jar:. GDALWarpThreadTest ../experiments/data/c41078a1.tif small
	GDALWARP_GC_LOCK=1 $(JAVA) -cp gdalwarp.jar:. GDALWarpThreadTest ../experiments/data/c41078a1.tif small

clean:
	rm -f *.class java/cz/adamh/utils/*.class java/com/azavea/gdal/*.class

cleaner: clean
	rm -f gdalwarp.jar
	rm -f java/resources/libgdalwarp_bindings.$(SO)

cleanest: cleaner
