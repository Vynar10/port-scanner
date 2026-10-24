CXX      = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -Iinclude
LDFLAGS  = -pthread

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=build/%.o)

scanner: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@ $(LDFLAGS)

build/%.o: src/%.cpp $(wildcard include/*.h) | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build scanner

.PHONY: clean
