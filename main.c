#include <gb/gb.h>
#include <gb/console.h>
#include <stdio.h>
#include <rand.h>

#include "Graphics/slots.c"
#include "Graphics/slotsmap.c"

#define TITLE_X 56
#define TITLE_Y 40

#define X_SLOT1 56
#define X_SLOT2 80
#define X_SLOT3 104

#define Y_SLOT 72

#define SPRITE_SCANLINE_START 54
#define SPRITE_SCANLINE_END 64

#define LCDC     (UBYTE*)0xFF40U
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

UBYTE slotStates[3];
UBYTE slotRunning[3];
UBYTE slotYpos[3];
UBYTE running = 0;
UBYTE nextDeactivateSlot = 0;

UBYTE inputLock = 0;
UBYTE updateCooldown = 0;

void updateSlotIcons() {
    for(UBYTE i = 0; i < 3; i++) set_sprite_tile(i, slotStates[i]);
}

void loadBackground() {
    set_bkg_data(0, 15, slots);
    set_bkg_tiles(5, 3, 10, 10, slotsmap);
}

void clss()  {
	for(UBYTE i = 0; i < 17; ++i) {
		gotoxy(0, i);
        if(i == 2) printf("       SLOTS       ");
        else if(!running && i == 14) printf("       PRESS       ");
        else if(!running && i == 15) printf("       START       ");
		else printf("                    ");
	}
    loadBackground();
    updateSlotIcons();
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

void updateSingleSlot(UBYTE slot) {
    if(slotRunning[slot] || slotYpos[slot] != 8) {

        if(!slotRunning[slot] && slotYpos[slot] < 8 && slotYpos[slot] + (slot + 1) > 8) slotYpos[slot] = 8;
        else slotYpos[slot] += (slot + 1);

        if(slotYpos[slot] >= 20) {
            slotYpos[slot] = 0;
            slotStates[slot] = (slotStates[slot] - 8) % 6 + 9;
            move_sprite(slot, slot == 0 ? X_SLOT1 : slot == 1 ? X_SLOT2 : X_SLOT3, Y_SLOT - 8);
        }
        else move_sprite(slot, slot == 0 ? X_SLOT1 : slot == 1 ? X_SLOT2 : X_SLOT3, Y_SLOT - (8 - slotYpos[slot]));
    }
    
}

void updateSlotRotation() {
    for(UBYTE i = 0; i < 3; i++) {
        updateSingleSlot(i);
    }
    updateSlotIcons();
}

void handleSlotStop() {
    gotoxy(0, 15);
    if(slotStates[0] == slotStates[1] && slotStates[1] == slotStates[2]) printf("        WIN        ");
    else  printf("     TRY AGAIN     ");
}

void main() {

    disable_interrupts();

    // Initialize Sprites
    set_sprite_data(0, 15, slots);

    for(UBYTE i = 0; i < 3; i++) {
        slotStates[i] = Seven;
        slotRunning[i] = 0;
        slotYpos[i] = 8;
        UBYTE x = i == 0 ? X_SLOT1 : i == 1 ? X_SLOT2 : X_SLOT3;
        move_sprite(i, x, Y_SLOT);
        set_sprite_prop(i, S_PRIORITY);
    }

    // Initialize Interrupts
    add_LCD(stat_isr);
    *LYC_REG = SPRITE_SCANLINE_START;
    *STAT_REG = 0x45;
    set_interrupts(LCD_IFLAG | VBL_IFLAG);

    // Start Game
    clss();
    enable_interrupts();
    SHOW_BKG;
    DISPLAY_ON;

    while(1) {

        UBYTE input = joypad();
        if(input & J_START) {
            if(!inputLock) {
                inputLock = 1;
                if(!running) {
                    nextDeactivateSlot = 0;
                    running = 1;
                    for(UBYTE i = 0; i < 3; i++) slotRunning[i] = 1;
                    clss();
                } else {
                    if(nextDeactivateSlot != 3) slotRunning[nextDeactivateSlot++] = 0;
                }
            }
        } else inputLock = 0;
        
        if(nextDeactivateSlot == 3) {
            UBYTE allStopped = 1;
            for(UBYTE i = 0; i < 3; i++) {
                allStopped = slotYpos[i] == 8;
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