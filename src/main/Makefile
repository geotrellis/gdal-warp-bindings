JAVA ?= java
JAVAC ?= javac

.PHONY: tests
.SILENT: tests

java/com/azavea/gdal/GDALWarp.class: java/com/azavea/gdal/GDALWarp.java java/cz/adamh/utils/NativeUtils.java
	$(JAVAC) -h .. -cp java $<

%.class: %.java
	$(JAVAC) -cp java $<

../libgdalwarp_bindings.so:
	make -C .. libgdalwarp_bindings.so

# MIT-Licensed code (that license is compatible with Apache 2.0)
java/cz/adamh/utils/NativeUtils.java:
	wget -k "https://raw.githubusercontent.com/adamheinrich/native-utils/e6a39489662846a77504634b6fafa4995ede3b1d/src/main/$@" -O $@

../experiments/data/c41078a1.tif:
	make -C ../experiments data/c41078a1.tif

tests: ../libgdalwarp_bindings.so java/com/azavea/gdal/GDALWarp.class java/com/azavea/gdal/GDALWarpThreadTest.class java/cz/adamh/utils/NativeUtils.class ../experiments/data/c41078a1.tif
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.. $(JAVA) -cp java com.azavea.gdal.GDALWarpThreadTest ../experiments/data/c41078a1.tif

clean:
	rm -f java/com/azavedal/*.class java/cz/adamh/utils/*.class

cleaner: clean

cleanest: cleaner
	rm -f java/cz/adamh/utils/NativeUtils.java
