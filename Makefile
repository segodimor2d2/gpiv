CC = gcc

PKGS = gtk4 mpv epoxy

CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags $(PKGS))
LDLIBS = $(shell pkg-config --libs $(PKGS)) -lGL

SRC = src/main.c
BIN = gpiv

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDLIBS)

clean:
	rm -f $(BIN)

.PHONY: all clean
