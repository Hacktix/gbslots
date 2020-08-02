#include <gb/gb.h>
#include <gb/console.h>
#include <gb/drawing.h>
#include <stdio.h>

#include "Graphics/slots.c"
#include "Graphics/slotsmap.c"

#define X_SLOT1 56
#define X_SLOT2 80
#define X_SLOT3 104

#define Y_SLOT 72

#define SPRITE_SCANLINE_START 54
#define SPRITE_SCANLINE_END 64

#define STAT_REG (UBYTE*)0xFF41U
#define LY_REG   (UBYTE*)0xFF44U
#define LYC_REG  (UBYTE*)0xFF45U

enum SlotIcon {
    Diamond = 9,
    Seven = 10,
    Watermelon = 11,
    Bell = 12,
    Lemon = 13,
    Cherry = 14
};

struct Slot {
    UBYTE state;
    UBYTE running;
    UBYTE ypos;
};

struct Slot gameSlots[3];

UBYTE running = 0;
UBYTE nextDeactivateSlot = 0;

UBYTE inputLock = 0;
UBYTE updateCooldown = 0;
UBYTE inputCooldown = 0;

UWORD credits = 10;

void updateSlotIcons() {
    for(UBYTE i = 0; i < 3; i++) set_sprite_tile(i, gameSlots[i].state);
}

void loadBackground() {
    set_bkg_data(0, 34, slots);
    set_bkg_tiles(5, 3, 32, 32, slotsmap);
}

void clss() {    
    loadBackground();
    updateSlotIcons();

    // Clear artifacts in tile row 1
    gotoxy(0,0);
    for(UBYTE i = 0; i < 20; i++) printf("\"");

    // Draw credits label
    gotoxy(4,10);
    printf("Credits:");
    gotoxy(13,10);
    printf("%d%c%c%c", credits, 0, 0, 0);
}

void handleSTAT() {
    switch(*LYC_REG) {
        case SPRITE_SCANLINE_START:
            SHOW_SPRITES;
            *LYC_REG = SPRITE_SCANLINE_END;
            break;
        case SPRITE_SCANLINE_END:
            HIDE_SPRITES;
            *LYC_REG = SPRITE_SCANLINE_START;
            break;
    }
}

void stat_isr() {
    handleSTAT();
}

void handleVblank() {
    UBYTE input = joypad();
    if(input & J_START) {
        if(!inputLock) {
            inputLock = 1;
            if(!running) {
                credits--;
                nextDeactivateSlot = 0;
                running = 1;
                for(UBYTE i = 0; i < 3; i++) gameSlots[i].running = 1;
                clss();
            } else {
                if(nextDeactivateSlot != 3) gameSlots[nextDeactivateSlot++].running = 0;
            }
        }
    } else inputLock = 0;
}

void vbl_isr() {
    handleVblank();
}

void updateSingleSlot(UBYTE slot) {
    if(gameSlots[slot].running || gameSlots[slot].ypos != 8) {

        if(!gameSlots[slot].running && gameSlots[slot].ypos < 8 && gameSlots[slot].ypos + (slot + 1) > 8) gameSlots[slot].ypos = 8;
        else gameSlots[slot].ypos += (slot + 1);

        if(gameSlots[slot].ypos >= 20) {
            gameSlots[slot].ypos = 0;
            gameSlots[slot].state = (gameSlots[slot].state - 8) % 6 + 9;
            move_sprite(slot, slot == 0 ? X_SLOT1 : slot == 1 ? X_SLOT2 : X_SLOT3, Y_SLOT - 8);
        }
        else move_sprite(slot, slot == 0 ? X_SLOT1 : slot == 1 ? X_SLOT2 : X_SLOT3, Y_SLOT - (8 - gameSlots[slot].ypos));
    }
    
}

void updateSlotRotation() {
    for(UBYTE i = 0; i < 3; i++) {
        updateSingleSlot(i);
    }
    updateSlotIcons();
}

void handleSlotStop() {
    
}

void main() {

    disable_interrupts();
    DISPLAY_OFF;

    // Initialize Sprites
    set_sprite_data(0, 15, slots);

    for(UBYTE i = 0; i < 3; i++) {
        gameSlots[i].state = Seven;
        gameSlots[i].running = 0;
        gameSlots[i].ypos = 8;
        UBYTE x = i == 0 ? X_SLOT1 : i == 1 ? X_SLOT2 : X_SLOT3;
        move_sprite(i, x, Y_SLOT);
        set_sprite_prop(i, S_PRIORITY);
    }

    // Initialize Interrupts
    add_LCD(stat_isr);
    *LYC_REG = SPRITE_SCANLINE_START;
    *STAT_REG = 0x45;

    add_VBL(vbl_isr);

    set_interrupts(LCD_IFLAG | VBL_IFLAG);

    // Start Game
    clss();
    clss(); // I don't know why but calling this twice makes it work
    enable_interrupts();
    SHOW_BKG;
    DISPLAY_ON;

    while(1) {        
        if(nextDeactivateSlot == 3) {
            UBYTE allStopped = 1;
            for(UBYTE i = 0; i < 3; i++) {
                allStopped = gameSlots[i].ypos == 8;
                if(!allStopped) break;
            }
            if(allStopped) {
                running = 0;
                handleSlotStop();
            }
        }

        if(running) {
            if(updateCooldown) updateCooldown--;
            else {
                updateCooldown = 250;
                updateSlotRotation();
            }
        }
    }
}