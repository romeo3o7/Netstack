C = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -g3 -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined
FILES = interface.c

build:$(FILES)
	mkdir -p ./build
	$(C) $(CFLAGS) $(FILES) -o ./build/interface
