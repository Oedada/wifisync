CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra
LDLIBS = -lssl -lcrypto

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp, build/%.o, $(SRCS))
app: $(OBJS)
	$(CXX) $(OBJS) -o app $(LDLIBS)

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clear
clear : $(OBJS)
	rm -rf $<
