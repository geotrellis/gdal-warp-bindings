CFLAGS ?= -Wall -Werror -Og -ggdb3
CXXFLAGS ?= -std=c++14
BOOST_ROOT ?= /usr/include

all: rawthread

%.o: %.cpp
	$(CXX) `pkg-config gdal --cflags` $(CFLAGS) $(CXXFLAGS) -I$(BOOST_ROOT) $< -c -o $@

rawthread: rawthread.o
	$(CC) $^ `pkg-config gdal --libs` -lstdc++ -lpthread -o $@

clean:
	rm -f *.o

cleaner: clean
	rm -f rawthread

cleanest: cleaner
