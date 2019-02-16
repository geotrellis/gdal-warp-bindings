CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
LDFLAGS += $(shell pkg-config gdal --libs) -l:libstdc++.a -lpthread
GDALCFLAGS = $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include

.PHONY: experiments/data

all: libgdalwarp_bindings.so

%.o: %.cpp
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

%_tests.o: %_tests.cpp types.hpp locked_dataset.hpp
	$(CXX) $(GDALCFLAGS) -I$(BOOST_ROOT) $(CFLAGS) $(CXXFLAGS) $< -c -o $@

bindings.o: bindings.cpp bindings.h types.hpp lru_cache.hpp locked_dataset.hpp
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

libgdalwarp_bindings.so: bindings.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

dataset_tests: dataset_tests.o
	$(CC) $(CFLAGS) $< $(LDFLAGS) -lm -o $@

experiments/data:
	make -C experiments data/c41078a1.tif

tests: experiments/data dataset_tests
	dataset_tests

clean:
	rm -f *.o

cleaner: clean
	rm -f libgdalwarp_bindings.so
	rm -f dataset_tests

cleanest: cleaner
