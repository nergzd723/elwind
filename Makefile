CC = clang

all:
	$(CC) elwind.c elwindInstructions.c elwindVisuals.c -O2 -g -lSDL2 -o elwind

run: all
	./elwind

.SILENT: all run