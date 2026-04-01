CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -pthread 
LDLIBS = -lssl -lcrypto -I/usr/include/SDL2 -D_GNU_SOURCE=1 -D_REENTRANT -lSDL2

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))
app: $(OBJS)
	$(CXX) $(OBJS) -o app $(LDLIBS)

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clear
clear : $(OBJS)
	rm -rf $<
