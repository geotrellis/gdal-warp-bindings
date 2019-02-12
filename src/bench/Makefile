CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++14
BOOST_ROOT ?= /usr/include

all: rawbench

%.o: %.cpp
	$(CXX) `pkg-config gdal --cflags` $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< -c -o $@

rawbench: rawbench.o
	$(CC) $^ `pkg-config gdal --libs` -lstdc++ -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f rawbench

cleanest: cleaner
