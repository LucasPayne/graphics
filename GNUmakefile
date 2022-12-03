
CC=g++
CFLAGS=-std=c++2a -Wextra -Wall -ggdb3 -march=native -Ilibraries/include/
LDFLAGS=-Llibraries/lib/ -Wl,-rpath=$(realpath libraries/lib/) -lglfw -lvulkan -ldl

build/main: main.cc
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm build/main
