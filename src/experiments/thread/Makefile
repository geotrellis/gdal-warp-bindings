CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++1z
BOOST_ROOT ?= /usr/include

all: rawthread wrapthread

%.o: %.cpp
	$(CXX) `pkg-config gdal --cflags` $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< -c -o $@

rawthread: rawthread.o
	$(CC) $^ `pkg-config gdal --libs` -lstdc++ -lboost_timer -lpthread -o $@

wrapthread: wrapthread.o ../../bindings.o ../../tokens.o
	$(CC) $^ `pkg-config gdal --libs` -lstdc++ -lboost_timer -lpthread -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f rawthread wrapthread

cleanest: cleaner
