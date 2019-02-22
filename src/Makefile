CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
LDFLAGS += $(shell pkg-config gdal --libs) -l:libstdc++.a -lpthread
JAVA_HOME ?= /usr/lib/jvm/java-11-openjdk-amd64
GDALCFLAGS = $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include
OS ?= linux
SO ?= so
HEADERS = bindings.h types.hpp lru_cache.hpp locked_dataset.hpp tokens.hpp

.PHONY: tests

all: tests libgdalwarp_bindings.$(SO)

com_azavea_gdal_GDALWarp.h: main/java/com/azavea/gdal/GDALWarp.java
	make -C main java/com/azavea/gdal/GDALWarp.class

com_azavea_gdal_GDALWarp.o: com_azavea_gdal_GDALWarp.c com_azavea_gdal_GDALWarp.h
	$(CC) $(CFLAGS) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(OS) -fPIC $< -c -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

bindings.o: bindings.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

libgdalwarp_bindings.$(SO): bindings.o tokens.o com_azavea_gdal_GDALWarp.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

experiments/data:
	make -C experiments data/c41078a1.tif

tests:
	make -C unit_tests tests
	make -C main/java tests

clean:
	rm -f *.o

cleaner: clean
	rm -f libgdalwarp_bindings.$(SO)
	rm -f dataset_tests token_tests cache_tests

cleanest: cleaner
	rm -f com_azavea_gdal_GDALWarp.h
