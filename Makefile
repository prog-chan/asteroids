CC = gcc
EMCC = emcc

CFLAGS = -std=c99 -Wall -Wextra -O2
RAYLIB = $(shell pkg-config --cflags --libs raylib)

WEB_RAYLIB = $(HOME)/raylib-web
WEB_FLAGS = -O2 -DPLATFORM_WEB -sUSE_GLFW=3
WEB_DIR = docs

main: main.c
	$(CC) $(CFLAGS) main.c -o main $(RAYLIB) -lm && ./main

web: main.c shell.html
	mkdir -p $(WEB_DIR)
	$(EMCC) main.c -o $(WEB_DIR)/index.html \
		$(WEB_FLAGS) \
		-I$(WEB_RAYLIB)/src \
		$(WEB_RAYLIB)/build-web/raylib/libraylib.a \
		--shell-file shell.html \
		-lm
	touch $(WEB_DIR)/.nojekyll

serve: web
	python3 -m http.server 8080 --directory $(WEB_DIR)

clean:
	rm -f main
	rm -f $(WEB_DIR)/index.html
	rm -f $(WEB_DIR)/index.js
	rm -f $(WEB_DIR)/index.wasm

.PHONY: main web serve clean
