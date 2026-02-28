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

// Animated scrolling diagonal stripes that advance one position each frame.
// Uses (i + scroll) % palette_size for a smooth, uniform directional scroll
// with no V-chevron artefact and no colour jump when scroll wraps (because
// the wrap value 600 is divisible by both 4 and 6).
void pattern_scroll_diagonal() {
  for (int i = 0; i < 24; i++) {
    uint16_t color = palette[(i + scroll) % palette_size];
    matrix.drawLine(0, 2*i,   2*i,   0, color);
    matrix.drawLine(0, 2*i+1, 2*i+1, 0, color);
  }
}

// Static diagonal stripes (no animation)
void pattern_diagonal() {
  for (int i = 0; i < 24; i++) {
    uint16_t color = palette[i % palette_size];
    matrix.drawLine(0, 2*i,   2*i,   0, color);
    matrix.drawLine(0, 2*i+1, 2*i+1, 0, color);
  }
}

// Solid colour blocks, one per digit column — best without colon shift
void pattern_blocks() {
  if (palette_size == 6) {
    matrix.fillRect(0,  0, 6, 11, palette[0]);
    matrix.fillRect(6,  0, 5, 11, palette[1]);
    matrix.fillRect(11, 0, 5, 11, palette[2]);
    matrix.fillRect(16, 0, 5, 11, palette[3]);
    matrix.fillRect(21, 0, 5, 11, palette[4]);
    matrix.fillRect(26, 0, 6, 11, palette[5]);
  }
  if (palette_size == 4) {
    for (int i = 0; i < 4; i++) {
      matrix.fillRect(i * 8, 0, 8, 11, palette[i]);
    }
  }
}

// Thin horizontal stripes, one stripe per palette colour per row
void pattern_h_thin() {
  for (int i = 0; i < 12; i++) {
    matrix.drawFastHLine(0, i, 32, palette[i % palette_size]);
  }
}

// Thick horizontal bands, evenly dividing the clock area between palette colours
void pattern_h_thick() {
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(0, i * (12 / palette_size), 32, 12 / palette_size, palette[i]);
  }
}

// Thin vertical stripes, one stripe per palette colour per column
void pattern_v_thin() {
  for (int i = 0; i < 32; i++) {
    matrix.drawFastVLine(i, 0, 11, palette[i % palette_size]);
  }
}

// Thick vertical bands, evenly dividing the 32-pixel display width among palette colours
void pattern_v_thick() {
  int band_w = 32 / palette_size;
  for (int i = 0; i < palette_size; i++) {
    matrix.fillRect(i * band_w, 0, band_w, 11, palette[i]);
  }
  // Fill any remaining pixels (due to integer division) with the last colour
  int covered = band_w * palette_size;
  if (covered < 32) {
    matrix.fillRect(covered, 0, 32 - covered, 11, palette[palette_size - 1]);
  }
}

// Random per-pixel colour from the active palette
void pattern_random() {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 16; y++) {
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

    case 9:  // Purple/Yellow contrast
      pal_swap(PURPLE, YELLOW, PURPLE, YELLOW);
      palette_size = 4;
      ink_swap(YELLOW, PURPLE, YELLOW, PURPLE); break;

    case 10: // Orange/Cyan contrast
      pal_swap(ORANGE, BLACK, BLUE, BLACK);
      palette_size = 4;
      ink_swap(BLUE, ORANGE, ORANGE, BLUE); break;

    case 11: // Monochrome (black/white) — grey digits
      pal_swap(BLACK, WHITE, BLACK, WHITE);
      palette_size = 4;
      ink_swap(GRAY, GRAY, GRAY, GRAY); break;
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
