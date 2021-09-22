#include "elwind.h"
#include <string.h>
#include <time.h>

void FillTileCache(ElwindMachine* machine){
    for(uint16_t tileIndex = 0; tileIndex < 256; tileIndex++){
        memcpy(machine->renderer.Tiles[tileIndex], machine->memory.VRAM+((tileIndex*16)+0x1000), 16);
    }
}

void PrintTile(ElwindMachine* machine, uint8_t tileno){
    uint8_t byte1, byte2;

    for(uint8_t row = 0; row < 8; row++){
        byte1 = machine->renderer.Tiles[tileno][row*2];
        byte2 = machine->renderer.Tiles[tileno][row*2+1];
        printf("row: 0x%x, 0x%x, 0x%x", row, byte1, byte2);
        for(uint8_t column = 8; column > 0; column--){
            if(byte1 & BIT(column) && byte2 & BIT(column)) printf("*");
            else printf(" ");
        }
        printf("\n");
    }
}

void PutPixelAt(ElwindMachine* machine, uint16_t x, uint16_t y, uint8_t color){
    memset(machine->renderer.Framebuffer+(x+(y*SIZE_Y))*PIXEL_SIZE, (UINT8_MAX/3)*color, PIXEL_SIZE);
}

void Draw(ElwindMachine* machine){
    SDL_UpdateTexture(machine->renderer.texture, NULL, machine->renderer.Framebuffer, SIZE_Y*sizeof(uint32_t));
    SDL_RenderClear(machine->renderer.renderer);
    SDL_RenderCopy(machine->renderer.renderer, machine->renderer.texture, NULL, NULL);
    SDL_RenderPresent(machine->renderer.renderer);
}

void UpdateBackgroundMap(ElwindMachine* machine){
    memcpy(machine->renderer.Background, machine->memory.VRAM+0x1800, 1024);
}

void DrawBackground(ElwindMachine* machine){
    clock_t begin = clock();

    UpdateBackgroundMap(machine);
    for (uint16_t i = 0; i < 1024; i++){
        DrawTileAt(machine, machine->renderer.Background[i], (i*8) % 256, (i / 32)*8);
        //if (machine->renderer.Background[i]) printf("x: 0x%x, y: 0x%x, tile: 0x%x\n", (i*8) % 256, ((i*8) / 256)*8, machine->renderer.Background[i]);
    }
    Draw(machine);
}

void DrawTileAt(ElwindMachine* machine, uint8_t tileno, uint16_t xoff, uint16_t yoff){
    uint8_t byte1, byte2;

    for(uint8_t row = 0; row < 8; row++){
        byte1 = machine->renderer.Tiles[tileno][row*2+1];
        byte2 = machine->renderer.Tiles[tileno][row*2];
        for(uint8_t column = 8; column > 0; column--){
            if(byte1 & BIT((8-column)) && byte2 & BIT((8-column))){
                PutPixelAt(machine, column+xoff, row+yoff, SHADE_WHITE);
            }
            else if(byte1 & BIT((8-column)) && !(byte2 & BIT((8-column)))){
                PutPixelAt(machine, column+xoff, row+yoff, SHADE_LIGHT_GREY);
            }
            else if(byte2 & BIT((8-column)) && !(byte1 & BIT((8-column)))){
                PutPixelAt(machine, column+xoff, row+yoff, SHADE_DARK_GREY);
            }
            else PutPixelAt(machine, column+xoff, row+yoff, SHADE_BLACK);
        }
    }
}

int InitSDL2(ElwindMachine* machine){
    if (SDL_Init(SDL_INIT_VIDEO)){
        printf("Error initializing SDL2: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Elwind", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SIZE_Y*2, SIZE_X*2, 0);
    if (!window) {
        printf("Could not create SDL2 window\n");
        return 1;
    }
    machine->renderer.window = window;
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        printf("Could not initialize SDL2 renderer\n");
        return 1;
    }
    machine->renderer.renderer = renderer;
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, SIZE_Y, SIZE_X);
    if (!texture){
        printf("Could not create a ARGB texture\n");
    }
    machine->renderer.texture = texture;
    memset(machine->renderer.Framebuffer, 255, SIZE_X*SIZE_Y*sizeof(uint32_t));

    SDL_UpdateTexture(texture, NULL, machine->renderer.Framebuffer, SIZE_Y*sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    return 0;
}
