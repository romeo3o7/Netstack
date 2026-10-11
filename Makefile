C = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2
FILES = interface.c

build:$(FILES)
	mkdir -p ./build
	$(C) $(CFLAGS) $(FILES) -o ./build/layerOne
