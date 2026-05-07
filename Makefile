# Simple makefile

CC := g++ -std=c++17 -O3

CCW := -Wall -Werror -Wextra

# C++ files needed to compile the project,
# resolves (/usr/bin/ld): undefined reference to
SOURCES := ./src/main.cpp

.DEFAULT_GOAL := compile

compile:
	mkdir -p bin
	$(CC) $(CCW) -o ./bin/main $(SOURCES)

clean:
	rm -rf ./bin/main

all: clean main
