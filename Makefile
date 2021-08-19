CC = gcc

all:
	$(CC) elwind.c elwindInstructions.c elwindVisuals.c elwindHardware.c -O3 -g -pedantic -lSDL2 -o elwind

run: all
	./elwind

.SILENT: all run