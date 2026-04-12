/*******************************************************************************
 * A layer 2 tilemap demo for Sinclair ZX Spectrum Next.
 ******************************************************************************/
#include <arch/zxn.h>
#include <arch/zxn/color.h>
#include <input.h>
#include <z80.h>
#include <intrinsic.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../config/zconfig.h"
#include "zx/bank.h"
#include "zx/layer2.h"
#include "zx/ula.h"
#include "zx/font.h"
#include "zx/sprite.h"
#include "zx/tilemap.h"
#include "zx/dma.h"
#include "zx/debug.h"
#include "zx/keyboard.h"
#include "globals.h"
#include "tiles.h"

static uint8_t buf_256[256];
static int key = 0;
static uint8_t direction = 0;


// * Function Prototypes
static void hardware_init(void);
static void isr_init(void);
static void create_background(void);


// * Functions
static void hardware_init(void)
{
  IO_7FFD = IO_7FFD_ROM0; // Make sure the Spectrum ROM is paged in initially.
  ZXN_NEXTREG(REG_TURBO_MODE, RTM_28MHZ); // Put Z80 in 28 MHz turbo mode.
  // Disable RAM memory contention.
  ZXN_NEXTREGA(REG_PERIPHERAL_3, ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION);
  
  ZXN_NEXTREG(REG_PALETTE_CONTROL, 0);
  ZXN_NEXTREG(REG_PALETTE_INDEX, 24);
  ZXN_NEXTREG(REG_PALETTE_VALUE_8, ZXN_RGB332_ZX_BRIGHT_MAGENTA);
  
  ZXN_NEXTREG(REG_TILEMAP_TRANSPARENCY_INDEX, 0);
  ZXN_NEXTREG(REG_GLOBAL_TRANSPARENCY_COLOR, ZXN_RGB332_ZX_BRIGHT_MAGENTA);
  ZXN_NEXTREG(REG_SPRITE_TRANSPARENCY_INDEX, ZXN_RGB332_ZX_BRIGHT_MAGENTA);
  
  ZXN_NEXTREGA(REG_SPRITE_LAYER_SYSTEM, RSLS_LAYER_PRIORITY_USL | RSLS_SPRITES_OVER_BORDER | RSLS_SPRITES_VISIBLE);
  
  ZXN_NEXTREG(REG_LAYER_2_RAM_PAGE, PAGE_LAYER2);
  
  IO_LAYER_2_CONFIG = IL2C_SHOW_LAYER_2;
  
  ZXN_NEXTREG(REG_CLIP_WINDOW_LAYER_2, 1);
  ZXN_NEXTREG(REG_CLIP_WINDOW_LAYER_2, 254);
  ZXN_NEXTREG(REG_CLIP_WINDOW_LAYER_2, 1);
  ZXN_NEXTREG(REG_CLIP_WINDOW_LAYER_2, 254);
  
  ZXN_NEXTREG(REG_LAYER_2_OFFSET_X_H, 0);
  ZXN_NEXTREG(REG_LAYER_2_OFFSET_X, 0);
  ZXN_NEXTREG(REG_LAYER_2_OFFSET_Y, 0);

  //ZXN_NEXTREG(REG_ULA_CONTROL, 0x80);
  
#if (SCREEN_RES == RES_256x192)
  ZXN_NEXTREG(REG_LAYER_2_CONTROL, LAYER_2_256x192x8);
#elif (SCREEN_RES == RES_320x256)
  ZXN_NEXTREG(REG_LAYER_2_CONTROL, LAYER_2_320x256x8);
#elif (SCREEN_RES == RES_640x256)
  ZXN_NEXTREG(REG_LAYER_2_CONTROL, LAYER_2_640x256x4);
#endif
}

static void isr_init(void)
{
  // Set up IM2 interrupt service routine:
  // Put Z80 in IM2 mode with a 257-byte interrupt vector table located
  // at 0x6000 (before CRT_ORG_CODE) filled with 0x61 bytes. Install an
  // empty interrupt service routine at the interrupt service routine
  // entry at address 0x8181.
#if (CRT_6000 == 1)
  intrinsic_di();
  im2_init((void *) 0x6000);
  memset((void *) 0x6000, 0x61, 257);
  z80_bpoke(0x6161, 0xFB);
  z80_bpoke(0x6162, 0xED);
  z80_bpoke(0x6163, 0x4D);
  intrinsic_ei();
#else
  intrinsic_di();
  im2_init((void *) 0x8000);
  memset((void *) 0x8000, 0x81, 257);
  z80_bpoke(0x8181, 0xFB);
  z80_bpoke(0x8182, 0xED);
  z80_bpoke(0x8183, 0x4D);
  intrinsic_ei();
#endif
}

static void background_create(void) {
  zx_cls(INK_WHITE | PAPER_BLACK | BRIGHT);
}

void init(void) {
  hardware_init();
  isr_init();
  background_create();
  layer2_clear_screen(0);
  sprites_clear();
  layer2_set_palette((const uint16_t *)&tiles_nxp, 128, 0);
  layer2_set_palette((const uint16_t *)&tiles_nxp + 128, 128, 128);
}

int main(void) {
  init();
  l2_map_tiles_x = 100;
  l2_map_tiles_y = 80;
  l2_map_data = (uint16_t *)&tiles_nxm;
  layer2_tilemap_update();
  
  while (true)
  {
    wait_vblank();
    scan_keyboard();
    zx_border(INK_RED);

//    layer2_draw_text(uint8_t row, uint8_t column, const char *text, uint8_t color);
    //              row col  text    color
 
    ula_print_char('D', 31, 2);
    ula_print_char('e', 32, 2);
    ula_print_char('m', 33, 2);
    ula_print_char('o', 34, 2);
    ula_print_char('!', 35, 2);

    if (KEY5 != 0) {
      layer2_tilemap_scroll_left();
    }

    key = in_inkey();
    switch(key) {
      case 'a': // Left
        layer2_tilemap_scroll_left();
        break;
      case 'd': // Right
        layer2_tilemap_scroll_right();
        break;
      case 'w': // Up
        layer2_tilemap_scroll_up();
        break;
      case 's': // Down
        layer2_tilemap_scroll_down();
        break;
      case 'r': // Refresh
        layer2_clear_screen(0);
        layer2_tilemap_update();
        break;
    }
    zx_border(INK_BLACK);
  }
  // Unreachable code: while(true) loop never exits
  // ZXN_NEXTREG(REG_RESET, RR_SOFT_RESET);
  // return 0;
}
