CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
LDFLAGS += $(shell pkg-config gdal --libs) -l:libstdc++.a -lpthread
JAVAC ?= javac
JAVA_HOME ?= /usr/lib/jvm/java-11-openjdk-amd64
GDALCFLAGS = $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include
HEADERS = bindings.h types.hpp lru_cache.hpp locked_dataset.hpp tokens.hpp

.PHONY: experiments/data tests

all: tests libgdalwarp_bindings.so

com_azavea_gdal_GDALWarp.h: ../jni/src/com/azavea/gdal/GDALWarp.java
	$(JAVAC) -h . $<

com_azavea_gdal_GDALWarp.o: com_azavea_gdal_GDALWarp.c com_azavea_gdal_GDALWarp.h
	$(CC) $(CFLAGS) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -fPIC $< -c -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

%_tests.o: %_tests.cpp  $(HEADERS)
	$(CXX) $(GDALCFLAGS) -I$(BOOST_ROOT) $(CFLAGS) $(CXXFLAGS) $< -c -o $@

bindings.o: bindings.cpp $(HEADERS)
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

libgdalwarp_bindings.so: bindings.o tokens.o com_azavea_gdal_GDALWarp.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

token_tests: token_tests.o tokens.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lm -o $@

dataset_tests: dataset_tests.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lm -o $@

cache_tests: cache_tests.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lm -o $@

experiments/data:
	make -C experiments data/c41078a1.tif

tests: experiments/data dataset_tests token_tests cache_tests
	dataset_tests
	token_tests
	cache_tests

clean:
	rm -f *.o

cleaner: clean
	rm -f libgdalwarp_bindings.so
	rm -f dataset_tests token_tests cache_tests

cleanest: cleaner
	rm -f com_azavea_gdal_GDALWarp.h
