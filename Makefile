CC=gcc
SRC=src/main.c
OUT=bin/beditor
CFLAGS=-Wall -Wextra -O2

all: build

build:
	$(CC) $(SRC) -o $(OUT) $(CFLAGS)


