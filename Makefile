CC = gcc

PKGS = gtk4 mpv epoxy

CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags $(PKGS))
LDLIBS = $(shell pkg-config --libs $(PKGS)) -lGL -lm

SRC = src/main.c src/app.c src/player.c
BIN = gpiv

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDLIBS)

clean:
	rm -f $(BIN)

.PHONY: all clean
