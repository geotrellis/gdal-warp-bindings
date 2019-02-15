CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
LDFLAGS += $(shell pkg-config gdal --libs) -l:libstdc++.a -lpthread
GDALCFLAGS = $(shell pkg-config gdal --cflags)

all: libgdalwarp_bindings.so

%.o: %.cpp
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

bindings.o: bindings.cpp bindings.h types.hpp lru_cache.hpp
	$(CXX) $(GDALCFLAGS) $(CFLAGS) $(CXXFLAGS) -fPIC $< -c -o $@

libgdalwarp_bindings.so: bindings.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -shared -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f libgdalwarp_bindings.so

cleanest: cleaner
