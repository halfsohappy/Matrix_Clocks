// face_task_list.h
// Background pattern functions and palette/ink management for ella_clock.
// This file is #included directly inside ella_clock.ino after all global
// variable declarations, so it can reference matrix, palette, scroll, etc.

// Convenience macros: set all four ink_color[] slots to black or white
#define BLACK_INK ink_swap(BLACK,BLACK,BLACK,BLACK)
#define WHITE_INK ink_swap(WHITE,WHITE,WHITE,WHITE)

// ---- BACKGROUND PATTERN CALLBACKS -----------------------------------------
// Each function fills the matrix with a colour pattern and is registered as
// the callback for face_task by switch_pattern().

// Animated scrolling diagonal (anti-diagonal, slope=-1) stripes.
// The full 32×10 time area is covered by iterating over every diagonal sum
// d = x+y in range 0..40 (0+0 to 31+9).  For each d the endpoints are
// clipped to the time rectangle so no pixel is drawn outside rows 0–9.
// Stripe colour index = d/2 so every two diagonals share a colour, giving
// 2-pixel-wide bands.  Scroll offsets the colour index; wrap at 600 is
// divisible by both 4 and 6 so there is no colour jump at the wrap point.
void pattern_scroll_diagonal() {
  for (int d = 0; d <= 40; d++) {
    uint16_t color = palette[((d / 2) + scroll) % palette_size];
    int x0 = (d <= 9)  ? 0    : d - 9;
    int y0 = (d <= 9)  ? d    : 9;
    int x1 = (d <= 31) ? d    : 31;
    int y1 = (d <= 31) ? 0    : d - 31;
    matrix.drawLine(x0, y0, x1, y1, color);
  }
}

// Static diagonal stripes — same layout as above but no animation.
void pattern_diagonal() {
  for (int d = 0; d <= 40; d++) {
    uint16_t color = palette[(d / 2) % palette_size];
    int x0 = (d <= 9)  ? 0    : d - 9;
    int y0 = (d <= 9)  ? d    : 9;
    int x1 = (d <= 31) ? d    : 31;
    int y1 = (d <= 31) ? 0    : d - 31;
    matrix.drawLine(x0, y0, x1, y1, color);
  }
}

// Solid colour blocks, one per digit column.  Height clamped to 10 rows
// (rows 0–9) so the date area is never painted by the pattern.
void pattern_blocks() {
  if (palette_size == 6) {
    matrix.fillRect(0,  0, 6, 10, palette[0]);
    matrix.fillRect(6,  0, 5, 10, palette[1]);
    matrix.fillRect(11, 0, 5, 10, palette[2]);
    matrix.fillRect(16, 0, 5, 10, palette[3]);
    matrix.fillRect(21, 0, 5, 10, palette[4]);
    matrix.fillRect(26, 0, 6, 10, palette[5]);
  }
  if (palette_size == 4) {
    for (int i = 0; i < 4; i++) {
      matrix.fillRect(i * 8, 0, 8, 10, palette[i]);
    }
  }
}

// Thin horizontal stripes, one per palette colour per row — rows 0–9 only.
void pattern_h_thin() {
  for (int i = 0; i < 10; i++) {
    matrix.drawFastHLine(0, i, 32, palette[i % palette_size]);
  }
}

// Thick horizontal bands filling rows 0–9, distributed evenly among palette
// colours with any remainder rows given to the last colour.
void pattern_h_thick() {
  int band_h = 10 / palette_size;
  if (band_h < 1) band_h = 1;
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(0, i * band_h, 32, band_h, palette[i]);
  }
  int covered = band_h * palette_size;
  if (covered < 10) {
    matrix.fillRect(0, covered, 32, 10 - covered, palette[palette_size - 1]);
  }
}

// Thin vertical stripes, one per palette colour per column — rows 0–9 only.
void pattern_v_thin() {
  for (int i = 0; i < 32; i++) {
    matrix.drawFastVLine(i, 0, 10, palette[i % palette_size]);
  }
}

// Thick vertical bands filling the 32-pixel width — rows 0–9 only.
void pattern_v_thick() {
  int band_w = 32 / palette_size;
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(i * band_w, 0, band_w, 10, palette[i]);
  }
  int covered = band_w * palette_size;
  if (covered < 32) {
    matrix.fillRect(covered, 0, 32 - covered, 10, palette[palette_size - 1]);
  }
}

// Random per-pixel colour from the active palette — rows 0–9 only.
void pattern_random() {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 10; y++) {
      matrix.drawPixel(x, y, palette[random(palette_size)]);
    }
  }
}

// ---- PATTERN SWITCHER ------------------------------------------------------

// Point draw_current_pattern at the chosen background pattern function.
// The main loop calls draw_current_pattern() every frame, so the back buffer
// is always fully repainted before digits are overlaid.
void switch_pattern(int pattern) {
  switch (pattern) {
    case 0: draw_current_pattern = &pattern_scroll_diagonal; break;
    case 1: draw_current_pattern = &pattern_diagonal;        break;
    case 2: draw_current_pattern = &pattern_blocks;          break;
    case 3: draw_current_pattern = &pattern_h_thin;          break;
    case 4: draw_current_pattern = &pattern_h_thick;         break;
    case 5: draw_current_pattern = &pattern_v_thin;          break;
    case 6: draw_current_pattern = &pattern_v_thick;         break;
    case 7: draw_current_pattern = &pattern_random;          break;
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

    case 11: // Monochrome — white digits on dark/mid-grey (avoids black-on-black
             // and white-on-white that pure BLACK/WHITE backgrounds caused)
      pal_swap(matrix.color565(15,15,15), matrix.color565(100,100,100),
               matrix.color565(15,15,15), matrix.color565(100,100,100));
      palette_size = 4; WHITE_INK; break;
  }
}

// Cycle to the next palette (wraps 11 → 1 to stay within valid range 1–11)
void change_pal_helper() {
  current_palette += 1;
  if (current_palette == 12) { current_palette = 1; }
}

// Cycle to the next pattern (wraps at 8 back to 0)
void change_pat_helper() {
  current_pattern += 1;
  if (current_pattern == 8) { current_pattern = 0; }
}

// Stop the background pattern
void disablePattern() {
  draw_current_pattern = nullptr;
}
