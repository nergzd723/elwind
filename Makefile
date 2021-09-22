CC = gcc

all:
	$(CC) elwind.c elwindInstructions.c elwindVisuals.c elwindHardware.c elwindJIT.c -O0 -g -lSDL2 -o elwind

run: all
	./elwind

.SILENT: all run