
CC=g++
CFLAGS_COMMON=-std=c++2a -Wextra -Wall -ggdb3 -march=native -Ilibraries/include/ -Llibraries/lib/ -lglfw -Wl,-rpath=$(realpath libraries/lib/)

build/main: main.cc
	$(CC) -o $@ $< $(CFLAGS_COMMON)

clean:
	rm build/main
