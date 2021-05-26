CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
GDALCFLAGS ?= $(shell pkg-config gdal --cflags)
LDFLAGS ?= $(shell pkg-config gdal --libs) -L$(shell pwd) -lgdalwarp_bindings-$(ARCH) -lstdc++ -lpthread
BOOST_ROOT ?= /usr/include
SO ?= so
ARCH ?= amd64

all: rawthread wrapthread pattern oversubscribe metadata

../../libgdalwarp_bindings-$(ARCH).$(SO):
	$(MAKE) -C ../.. libgdalwarp_bindings-$(ARCH).$(SO)

libgdalwarp_bindings-$(ARCH).$(SO): ../../libgdalwarp_bindings-$(ARCH).$(SO)
	ln -s $< $@

rawthread: rawthread.o libgdalwarp_bindings-$(ARCH).$(SO)
	$(CC) $< $(LDFLAGS) -lboost_timer -o $@

wrapthread: wrapthread.o libgdalwarp_bindings-$(ARCH).$(SO)
	$(CC) $< $(LDFLAGS) -lboost_timer -o $@

pattern: pattern.o libgdalwarp_bindings-$(ARCH).$(SO)
	$(CC) $< $(LDFLAGS) -o $@

oversubscribe: oversubscribe.o libgdalwarp_bindings-$(ARCH).$(SO)
	$(CC) $< $(LDFLAGS) -o $@

metadata: metadata.o libgdalwarp_bindings-$(ARCH).$(SO)
	$(CC) $< $(LDFLAGS) -lboost_timer -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(GDALCFLAGS) -I$(BOOST_ROOT) $< -c -o $@

clean:
	rm -f *.o  libgdalwarp_bindings-$(ARCH).$(SO)

cleaner: clean
	rm -f rawthread wrapthread pattern oversubscribe metadata

cleanest: cleaner
