C = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2
FILES = layerOne.c

build:$(FILES)
	mkdir -p ./build
	$(C) $(CFLAGS) $(FILES) -o ./build/layerOne
