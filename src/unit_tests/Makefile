CFLAGS ?= -Wall -Werror -O0 -ggdb3
CFLAGS += -Wno-sign-compare -I..
CXXFLAGS ?= -std=c++14
LDFLAGS += $(shell pkg-config gdal --libs) -L.. -lgdalwarp_bindings-$(ARCH) -lpthread
GDALCFLAGS ?= $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include
SO ?= so
ARCH ?= amd64


.PHONY: tests clean cleaner cleanest
.SILENT: tests

tests: ../libgdalwarp_bindings-$(ARCH).$(SO) ../experiments/data/c41078a1.tif token_tests dataset_tests cache_tests bindings_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./token_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./dataset_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./cache_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./bindings_tests

../experiments/data/c41078a1.tif:
	$(MAKE) -C ../experiments data/c41078a1.tif

../libgdalwarp_bindings-$(ARCH).$(SO):
	$(MAKE) -C .. libgdalwarp_bindings.$(SO)

token_tests: token_tests.cpp ../libgdalwarp_bindings-$(ARCH).$(SO)
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

dataset_tests: dataset_tests.cpp ../libgdalwarp_bindings-$(ARCH).$(SO)
	$(CXX) $(CFLAGS) $(GDALCFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

cache_tests: cache_tests.cpp ../libgdalwarp_bindings-$(ARCH).$(SO)
	$(CXX) $(CFLAGS) $(GDALCFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

bindings_tests: bindings_tests.cpp ../libgdalwarp_bindings-$(ARCH).$(SO)
	$(CXX) $(CFLAGS) $(GDALCFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

clean:
	rm -f *_tests

cleaner: clean

cleanest: cleaner
