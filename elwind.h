#ifndef ELWIND_H
#define ELWIND_H

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PIXEL_SIZE 4 // ARGB, 4 bytes

#define PC_START 0x100
#define ROM_BASE_SIZE 0x4000

#define LCDC_CTRL 0x40
#define LCDC_Y 0x44

#define SERIAL_WRITE 0x1
#define SERIAL_CTRL 0x2
#define SERIAL_TRANSFER_START 7
#define SERIAL_CLOCK_SPEED 1
#define SERIAL_CLOCKSOURCE 0

#define LCDC_CTRL_EN 7
#define LCDC_CTRL_TILEMAP_MAP 6
#define LCDC_CTRL_WINDOW_EN 5
#define LCDC_CTRL_BG_SELECT 4
#define LCDC_CTRL_BG_TILEMAP 3
#define LCDC_CTRL_SPRITE_SIZE 2
#define LCDC_CTRL_SPRITE_EN 1
#define LCDC_CTRL_BG_WINDOW_PRIORITY 0

#define SIZE_X 256
#define SIZE_Y 256

#define FLAG_ZERO 7
#define FLAG_NEGATIVE 6
#define FLAG_HC 5
#define FLAG_CARRY 4

#define BIT(n) (1 << n)

enum MBCMode{
    MBC_MODE_NONE,
    MBC_MODE_MBC1,
    MBC_MODE_MBC2,
    MBC_MODE_MBC3,
    MBC_MODE_MBC5,
};

typedef struct {
    union {
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };
    union {
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };
    union {
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };
    union {
        struct {
            uint8_t f;
            uint8_t a;
        };
        uint16_t af;
    };
    uint16_t sp;
    uint16_t pc;
} ElwindRegisters;

typedef struct {
    uint8_t Memory[0xFFFF];
    uint8_t VRAM[0x2000];
    uint8_t OAM[0xFF];
    uint8_t IOMemory[0x7F];
    enum MBCMode banking_mode;
} ElwindMemory;

typedef struct {
    uint8_t Tiles[256][16];
    uint8_t Background[1024];
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint8_t Framebuffer[SIZE_Y*SIZE_X*PIXEL_SIZE + PIXEL_SIZE];
} ElwindRenderer;

typedef struct {
    ElwindRegisters registers;
    ElwindMemory memory;
    ElwindRenderer renderer;
    FILE* ElwindROM;
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
void mbc1_handler(ElwindMachine* machine, uint16_t address, uint8_t val);
void setupCodegen();
void runJIT(ElwindMachine* machine);
void ProcessInterrupt(ElwindMachine* machine);

#endif
