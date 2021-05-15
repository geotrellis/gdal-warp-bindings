CFLAGS ?= -Wall -Werror -Og -ggdb3 -D_GNU_SOURCE
CXXFLAGS ?= -std=c++14
LDFLAGS ?= $(shell pkg-config gdal --libs) -l:libstdc++.a -lpthread
JAVA_HOME ?= /usr/lib/jvm/java-11-openjdk-amd64
GDALCFLAGS ?= $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include
OS ?= linux
SO ?= so
HEADERS = bindings.h types.hpp flat_lru_cache.hpp locked_dataset.hpp tokens.hpp errorcodes.hpp

all: tests libgdalwarp_bindings.$(SO)

com_azavea_gdal_GDALWarp.o: com_azavea_gdal_GDALWarp.c
	$(MAKE) -C main java/com/azavea/gdal/GDALWarp.class
	$(CC) $(CFLAGS) $(GDALCFLAGS) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(OS) -fPIC $< -c -o $@

com_azavea_gdal_GDALWarp.obj: com_azavea_gdal_GDALWarp.c
	$(MAKE) -C main java/com/azavea/gdal/GDALWarp.class
	$(CC) $(CFLAGS) $(GDALCFLAGS) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(OS) -fPIC $< -c -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) -fPIC $< -c -o $@

%.obj: %.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) -fPIC $< -c -o $@

libgdalwarp_bindings.$(SO): com_azavea_gdal_GDALWarp.o bindings.o tokens.o errorcodes.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

gdalwarp_bindings.dll: com_azavea_gdal_GDALWarp.obj bindings.obj tokens.obj errorcodes.obj
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

experiments/data:
	$(MAKE) -C experiments data/c41078a1.tif

tests: libgdalwarp_bindings.$(SO)
	$(MAKE) -C unit_tests tests
	$(MAKE) -C main tests

clean:
	rm -f *.o
	$(MAKE) -C experiments clean
	$(MAKE) -C unit_tests clean
	$(MAKE) -C main clean

cleaner: clean
	rm -f libgdalwarp_bindings.$(SO) com_azavea_gdal_GDALWarp.h
	$(MAKE) -C experiments cleaner
	$(MAKE) -C unit_tests cleaner
	$(MAKE) -C main cleaner

cleanest: cleaner
	$(MAKE) -C experiments cleanest
	$(MAKE) -C unit_tests cleanest
	$(MAKE) -C main cleanest
