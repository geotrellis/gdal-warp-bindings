CFLAGS ?= -Wall -Werror -Og -ggdb3 -I..
CXXFLAGS ?= -std=c++1z
LDFLAGS += $(shell pkg-config gdal --libs) -L.. -lgdalwarp_bindings -lpthread
GDALCFLAGS = $(shell pkg-config gdal --cflags)
BOOST_ROOT ?= /usr/include
SO ?= so

.PHONY: tests
.SILENT: tests

tests: ../libgdalwarp_bindings.$(SO) ../experiments/data/c41078a1.tif token_tests dataset_tests cache_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./token_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./dataset_tests
	LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):.. ./cache_tests

../experiments/data/c41078a1.tif:
	make -C ../experiments data/c41078a1.tif

../libgdalwarp_bindings.$(SO):
	make -C .. libgdalwarp_bindings.$(SO)

token_tests: token_tests.cpp ../libgdalwarp_bindings.$(SO)
	$(CXX) $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

dataset_tests: dataset_tests.cpp ../libgdalwarp_bindings.$(SO)
	$(CXX) $(CFLAGS) $(GDALCFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

cache_tests: cache_tests.cpp ../libgdalwarp_bindings.$(SO)
	$(CXX) $(CFLAGS) $(GDALCFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< $(LDFLAGS) -lm -o $@

clean:
	rm -f *_tests

cleaner: clean

cleanest: cleaner
