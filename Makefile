CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -pedantic

all: usensor-submission

usensor-submission: src/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f usensor-submission

.PHONY: all clean
