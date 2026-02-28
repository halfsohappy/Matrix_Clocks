// face_task_list.h
// Background pattern functions and palette/ink management for matrix_clock.
// This file is #included directly inside matrix_clock.ino after all global
// variable declarations, so it can reference matrix, palette, scroll, etc.

// Convenience macros: set all four ink_color[] slots to black or white
#define BLACK_INK ink_swap(BLACK,BLACK,BLACK,BLACK)
#define WHITE_INK ink_swap(WHITE,WHITE,WHITE,WHITE)

// ---- BACKGROUND PATTERN CALLBACKS -----------------------------------------
// Each function fills the matrix with a colour pattern and is registered as
// the callback for face_task by switch_pattern().

// Animated scrolling anti-diagonal stripes covering the full 32×16 display.
// d = x+y ranges 0..46 (0+0 to 31+15).  Endpoints are clipped to the display
// rectangle.  Every two diagonals share a colour (2-pixel-wide bands).
// Scroll offsets the colour index; wrap at 600 keeps no colour jump.
void pattern_scroll_diagonal() {
  for (int d = 0; d <= 46; d++) {
    uint16_t color = palette[((d / 2) + scroll) % palette_size];
    int x0 = (d <= 15) ? 0    : d - 15;
    int y0 = (d <= 15) ? d    : 15;
    int x1 = (d <= 31) ? d    : 31;
    int y1 = (d <= 31) ? 0    : d - 31;
    matrix.drawLine(x0, y0, x1, y1, color);
  }
}

// Static anti-diagonal stripes — same layout as above but no animation.
void pattern_diagonal() {
  for (int d = 0; d <= 46; d++) {
    uint16_t color = palette[(d / 2) % palette_size];
    int x0 = (d <= 15) ? 0    : d - 15;
    int y0 = (d <= 15) ? d    : 15;
    int x1 = (d <= 31) ? d    : 31;
    int y1 = (d <= 31) ? 0    : d - 31;
    matrix.drawLine(x0, y0, x1, y1, color);
  }
}

// Solid colour blocks spanning the full 32×16 display.
void pattern_blocks() {
  if (palette_size == 6) {
    matrix.fillRect(0,  0, 6, 16, palette[0]);
    matrix.fillRect(6,  0, 5, 16, palette[1]);
    matrix.fillRect(11, 0, 5, 16, palette[2]);
    matrix.fillRect(16, 0, 5, 16, palette[3]);
    matrix.fillRect(21, 0, 5, 16, palette[4]);
    matrix.fillRect(26, 0, 6, 16, palette[5]);
  }
  if (palette_size == 4) {
    for (int i = 0; i < 4; i++) {
      matrix.fillRect(i * 8, 0, 8, 16, palette[i]);
    }
  }
}

// Thin horizontal stripes cycling through palette colours, full 16 rows.
void pattern_h_thin() {
  for (int i = 0; i < 16; i++) {
    matrix.drawFastHLine(0, i, 32, palette[i % palette_size]);
  }
}

// Thick horizontal bands filling all 16 rows, evenly distributed.
void pattern_h_thick() {
  int band_h = 16 / palette_size;
  if (band_h < 1) band_h = 1;
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(0, i * band_h, 32, band_h, palette[i]);
  }
  int covered = band_h * palette_size;
  if (covered < 16) {
    matrix.fillRect(0, covered, 32, 16 - covered, palette[palette_size - 1]);
  }
}

// Thin vertical stripes, one colour per column, full 16-row height.
void pattern_v_thin() {
  for (int i = 0; i < 32; i++) {
    matrix.drawFastVLine(i, 0, 16, palette[i % palette_size]);
  }
}

// Thick vertical bands across the full 32×16 display.
void pattern_v_thick() {
  int band_w = 32 / palette_size;
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(i * band_w, 0, band_w, 16, palette[i]);
  }
  int covered = band_w * palette_size;
  if (covered < 32) {
    matrix.fillRect(covered, 0, 32 - covered, 16, palette[palette_size - 1]);
  }
}

// Random per-pixel colour from the active palette, full 16 rows.
void pattern_random() {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 16; y++) {
      matrix.drawPixel(x, y, palette[random(palette_size)]);
    }
  }
}

// Animated horizontal bands that slide upward each scroll step.
// Each row gets the palette colour for (row + scroll) % palette_size,
// so the colour assigned to each row changes over time.
void pattern_scroll_h() {
  for (int y = 0; y < 16; y++) {
    matrix.drawFastHLine(0, y, 32, palette[(y + scroll) % palette_size]);
  }
}

// Animated 2×2 checkerboard that shifts colour every few scroll steps.
// Each 2×2 cell is assigned palette[(cell_x + cell_y + scroll/2) % palette_size].
void pattern_checker() {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 16; y++) {
      int cell = (x / 2) + (y / 2);
      matrix.drawPixel(x, y, palette[(cell + scroll / 2) % palette_size]);
    }
  }
}

// Animated bright bar that ping-pongs left-to-right across a solid base.
// The bar is 4 px wide; palette[0] fills the base, palette[1] paints the bar.
void pattern_bounce() {
  matrix.fillRect(0, 0, 32, 16, palette[0]);
  int period = 56;                             // total steps for one full round trip
  int raw    = scroll % period;
  int pos    = (raw < 28) ? raw : 55 - raw;   // ping-pong: 0 → 27 → 0
  uint16_t bar_color = palette[1 % palette_size];
  for (int w = 0; w < 4 && (pos + w) < 32; w++) {
    matrix.drawFastVLine(pos + w, 0, 16, bar_color);
  }
}

// Dark base filled with palette[0], then ~15 random sparkle pixels per frame
// drawn in any other palette colour.  Twinkle rate follows the scroll divider.
void pattern_sparkle() {
  matrix.fillRect(0, 0, 32, 16, palette[0]);
  if (palette_size > 1) {
    for (int i = 0; i < 15; i++) {
      matrix.drawPixel(random(32), random(16),
                       palette[1 + random(palette_size - 1)]);
    }
  }
}

// ---- PATTERN SWITCHER ------------------------------------------------------

// Point draw_current_pattern at the chosen background pattern function.
// The main loop calls draw_current_pattern() every frame, so the back buffer
// is always fully repainted before digits are overlaid.
void switch_pattern(int pattern) {
  switch (pattern) {
    case 0:  draw_current_pattern = &pattern_scroll_diagonal; break;
    case 1:  draw_current_pattern = &pattern_diagonal;        break;
    case 2:  draw_current_pattern = &pattern_blocks;          break;
    case 3:  draw_current_pattern = &pattern_h_thin;          break;
    case 4:  draw_current_pattern = &pattern_h_thick;         break;
    case 5:  draw_current_pattern = &pattern_v_thin;          break;
    case 6:  draw_current_pattern = &pattern_v_thick;         break;
    case 7:  draw_current_pattern = &pattern_random;          break;
    case 8:  draw_current_pattern = &pattern_scroll_h;        break;
    case 9:  draw_current_pattern = &pattern_checker;         break;
    case 10: draw_current_pattern = &pattern_bounce;          break;
    case 11: draw_current_pattern = &pattern_sparkle;         break;
  }
}

// ---- PALETTE AND INK HELPERS -----------------------------------------------

// Load up to 6 colours into the active palette
void pal_swap(uint16_t zero, uint16_t one, uint16_t two, uint16_t three,
              uint16_t four = 0, uint16_t five = 0) {
  palette[0] = zero;  palette[1] = one;  palette[2] = two;
  palette[3] = three; palette[4] = four; palette[5] = five;
}

// Set the per-digit ink colour for all four digit positions
void ink_swap(uint16_t zero, uint16_t one, uint16_t two, uint16_t three) {
  ink_color[0] = zero; ink_color[1] = one;
  ink_color[2] = two;  ink_color[3] = three;
}

// Apply the palette indexed by current_palette to palette[] and ink_color[]
void change_palette() {
  switch (current_palette) {
    case 1:  // Rainbow — black digits
      pal_swap(RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE);
      palette_size = 6; BLACK_INK; break;

    case 2:  // Rainbow — white digits
      pal_swap(RED, ORANGE, YELLOW, GREEN, BLUE, PURPLE);
      palette_size = 6; WHITE_INK; break;

    case 3:  // Red/Blue/Yellow/White — black digits
      pal_swap(RED, BLUE, YELLOW, WHITE);
      palette_size = 4; BLACK_INK; break;

    case 4:  // Pure RGB + White — black digits
      pal_swap(PURE_RED, PURE_GREEN, PURE_BLUE, WHITE);
      palette_size = 4; BLACK_INK; break;

    case 5:  // CMY + Black (CMYK-like) — white digits
      pal_swap(CYAN, MAGENTA, YELLOW, BLACK);
      palette_size = 4; WHITE_INK; break;

    case 6:  // Pastel pink/green — black digits
      pal_swap(matrix.color565(204,232,219), matrix.color565(193,212,227),
               matrix.color565(190,180,214), matrix.color565(250,218,226),
               matrix.color565(248,179,202), matrix.color565(204,151,193));
      palette_size = 6; BLACK_INK; break;

    case 7:  // "Wilderness" earth tones — black digits
      pal_swap(matrix.color565(63,53,53),   matrix.color565(169,92,74),
               matrix.color565(214,175,116), matrix.color565(135,163,100),
               matrix.color565(74,138,118),  matrix.color565(61,80,112));
      palette_size = 6; BLACK_INK; break;

    case 8:  // Duke University palette — black digits
      pal_swap(DUKE_BLUE, GRAY, WHITE, DUKE_BLUE);
      palette_size = 4; BLACK_INK; break;

    case 9:  // Purple/Yellow contrast — white digits (visible on both colours)
      pal_swap(PURPLE, YELLOW, PURPLE, YELLOW);
      palette_size = 4; WHITE_INK; break;

    case 10: // Orange/Blue contrast — white digits (visible on all four colours)
      pal_swap(ORANGE, BLACK, BLUE, BLACK);
      palette_size = 4; WHITE_INK; break;

    case 11: // Monochrome — white digits on dark/mid-grey
      pal_swap(matrix.color565(15,15,15), matrix.color565(100,100,100),
               matrix.color565(15,15,15), matrix.color565(100,100,100));
      palette_size = 4; WHITE_INK; break;

    case 12: // "Neon Night" — deep violet / hot pink / neon green / electric blue
      pal_swap(matrix.color565(10,  0,   60),   matrix.color565(255, 0,   120),
               matrix.color565(0,   200, 80),   matrix.color565(0,   80,  255));
      palette_size = 4; WHITE_INK; break;

    case 13: // "Ember" — dark red / burnt orange / dark amber / deep purple
      pal_swap(matrix.color565(120, 10,  0),    matrix.color565(200, 70,  0),
               matrix.color565(160, 100, 0),    matrix.color565(60,  0,   70));
      palette_size = 4; WHITE_INK; break;

    case 14: // "Deep Ocean" — navy / ocean blue / deep teal / dark blue
      pal_swap(matrix.color565(0,   20,  100),  matrix.color565(0,   80,  180),
               matrix.color565(0,   160, 140),  matrix.color565(0,   40,  80));
      palette_size = 4; WHITE_INK; break;

    case 15: // "Candy" — raspberry / violet / sky blue / mint
      pal_swap(matrix.color565(220, 0,   100),  matrix.color565(160, 0,   200),
               matrix.color565(0,   140, 220),  matrix.color565(0,   180, 130));
      palette_size = 4; WHITE_INK; break;
  }
}

// Cycle to the next palette (wraps 15 → 1 to stay within valid range 1–15)
void change_pal_helper() {
  current_palette += 1;
  if (current_palette == 16) { current_palette = 1; }
}

// Cycle to the next pattern (wraps at 12 back to 0)
void change_pat_helper() {
  current_pattern += 1;
  if (current_pattern == 12) { current_pattern = 0; }
}

// Stop the background pattern
void disablePattern() {
  draw_current_pattern = nullptr;
}
