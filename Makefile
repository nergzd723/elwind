CC = clang

all:
	$(CC) elwind.c elwindInstructions.c -O2 -g -o elwind

run: all
	./elwind

.SILENT: all run