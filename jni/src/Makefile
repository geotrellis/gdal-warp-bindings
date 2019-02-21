JAVA ?= java
JAVAC ?= javac

.PHONY: tests
.SILENT: tests

tests: ../../src/libgdalwarp_bindings.so com/azavea/gdal/GDALWarp.class com/azavea/gdal/GDALWarpThreadTest.class ../../src/experiments/data/c41078a1.tif
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../../src $(JAVA) com.azavea.gdal.GDALWarpThreadTest ../../src/experiments/data/c41078a1.tif

%.class: %.java
	$(JAVAC) $<

../../src/experiments/data/c41078a1.tif:
	make -C ../../src/experiments data/c41078a1.tif

../../src/libgdalwarp_bindings.so com/azavea/gdal/GDALWarp.class:
	make -C ../../src libgdalwarp_bindings.so

clean:
	rm -f com/azavea/gdal/*.class

cleaner: clean

cleanest: cleaner
