#ifndef ELWIND_H
#define ELWIND_H

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PIXEL_SIZE 4 // ARGB, 4 bytes

#define PC_START 0x100
#define ROM_SIZE 0x8000

#define LCDC_Y 0x44
#define SIZE_X 256
#define SIZE_Y 256

#define FLAG_ZERO 7
#define FLAG_NEGATIVE 6
#define FLAG_HC 5
#define FLAG_CARRY 4

#define BIT(n) (1 << n)


typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint8_t f;
    uint16_t sp;
    uint16_t pc;
} ElwindRegisters;

typedef struct {
    uint8_t Memory[0xFFFF];
    uint8_t VRAM[0x2000];
    uint8_t OAM[0xFF];
    uint8_t IOMemory[0x7F];
} ElwindMemory;

typedef struct {
    uint8_t Tiles[256][16];
    uint8_t Background[1024];
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint8_t Framebuffer[SIZE_Y*SIZE_X*4];
} ElwindRenderer;

typedef struct {
    ElwindRegisters registers;
    ElwindMemory memory;
    ElwindRenderer renderer;
} ElwindMachine;

typedef struct {
    char* info;
    uint8_t length;
    void (*Execute)(ElwindMachine*);
} ElwindInstruction;

enum Shade{
    SHADE_WHITE,
    SHADE_LIGHT_GREY,
    SHADE_DARK_GREY,
    SHADE_BLACK
};

extern const ElwindInstruction Instructions[256];
extern const ElwindInstruction PrefixedInstructions[256];

static char broken;
static char stopped = 1;

void FillTileCache(ElwindMachine* machine);
void PrintTile(ElwindMachine* machine, uint8_t tileno);
int InitSDL2(ElwindMachine* machine);
void DrawTileAt(ElwindMachine* machine, uint8_t tileno, uint16_t xoff, uint16_t yoff);
void DrawBackground(ElwindMachine* machine);

#endif
