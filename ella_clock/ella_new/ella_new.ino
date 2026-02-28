// ella_new.ino
// A rewritten RGB LED matrix clock using a two-layer pixel buffer system:
//   base_pixels[][] : holds the background colour for every pixel
//   top_pixels[][]  : holds the foreground colour used when drawing digit shapes
//
// This allows the background to change independently of the digit rendering.
// The hardware and display layout are the same as ella_clock:
//   32×16 matrix, rows 0-9 for the clock, row 10 separator, rows 11-15 date.

#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"

// Colour aliases
#define RED   colors[0]
#define ORANGE colors[1]
#define YELLOW colors[2]
#define GREEN colors[3]
#define BLUE  colors[4]
#define PURPLE colors[5]
#define BLACK colors[6]
#define WHITE colors[7]
#define GRAY  colors[8]
#define DUKE_BLUE colors[9]
#define CYAN  colors[10]
#define MAGENTA colors[11]

// MATRIX PINS (Metro/Feather M4 or compatible):
// uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
// uint8_t addrPins[] = {17, 18, 19, 20, 21};
// uint8_t clockPin   = 14;
// uint8_t latchPin   = 15;
// uint8_t oePin      = 16;
// int rot = 2;

// RP2040 Feather pins:
uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
uint8_t addrPins[] = {25, 24, 29, 28};
uint8_t clockPin   = 13;
uint8_t latchPin   = 1;
uint8_t oePin      = 0;
int rot = 0;

// Adafruit Protomatter matrix: 32 px wide, 4-bit colour depth, single chain,
// 3 address pins (height inferred as 16), no double-buffering
Adafruit_Protomatter matrix(
  32,          // Width of matrix in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, RGB pins
  3, addrPins, // # of address pins, address pin array
  clockPin, latchPin, oePin,
  false        // No double-buffering
);

// Named colour palette
uint16_t colors[] = {
  matrix.color565(255, 0, 0),      // 0 Red
  matrix.color565(253, 152, 0),    // 1 Orange
  matrix.color565(255, 255, 0),    // 2 Yellow
  matrix.color565(51, 254, 0),     // 3 Green
  matrix.color565(0, 151, 253),    // 4 Blue
  matrix.color565(102, 51, 253),   // 5 Purple
  0,                               // 6 Black
  65535,                           // 7 White
  matrix.color565(72, 72, 72),     // 8 Gray
  matrix.color565(28, 33, 168),    // 9 Duke Blue
  matrix.color565(0, 255, 255),    // 10 Cyan
  matrix.color565(255, 0, 255),    // 11 Magenta
};

// Two-layer pixel buffers (background and foreground colour maps)
uint16_t base_pixels[16][32]; // background colour for each pixel
uint16_t top_pixels[16][32];  // foreground colour used when drawing digit glyphs

// Real-time clock and current timestamp
RTC_DS3231 rtc;
DateTime now;

// time_hhmm[0..3] = [H1, H2, M1, M2] as digit indices 0-9
int time_hhmm[4] = {3, 1, 4, 1};

// Scroll counter for animating patterns
int scroll = 0;

// Month lookup table: each entry holds three letter indices (A=0 … Z=25)
// for the 3-char month abbreviation shown in the date row
int months[12][3] = {
  {9,0,13}, {5,4,1}, {12,0,17}, {0,15,17}, {12,0,24}, {9,20,13},
  {9,20,11}, {0,20,6}, {18,4,15}, {14,2,19}, {13,14,21}, {3,4,2}
};

// Small 3×5 numeric glyphs (indices 0-9) stored as bool[5][3]
bool small_num[10][5][3] = {
  {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // 0
  {{1,1,0},{0,1,0},{0,1,0},{0,1,0},{1,1,1}}, // 1
  {{1,1,1},{0,0,1},{0,1,0},{1,0,0},{1,1,1}}, // 2
  {{1,1,1},{0,0,1},{1,1,1},{0,0,1},{1,1,1}}, // 3
  {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}}, // 4
  {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}}, // 5
  {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}}, // 6
  {{1,1,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1}}, // 7
  {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}}, // 8
  {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}}  // 9
};

// Small 3×5 uppercase letter glyphs (A=0 … Z=25) stored as bool[5][3]
bool letters[26][5][3] = {
  {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,0,1}}, // A
  {{1,1,1},{1,0,1},{1,1,0},{1,0,1},{1,1,1}}, // B
  {{1,1,1},{1,0,0},{1,0,0},{1,0,0},{1,1,1}}, // C
  {{1,1,0},{1,0,1},{1,0,1},{1,0,1},{1,1,0}}, // D
  {{1,1,1},{1,0,0},{1,1,1},{1,0,0},{1,1,1}}, // E
  {{1,1,1},{1,0,0},{1,1,1},{1,0,0},{1,0,0}}, // F
  {{1,1,1},{1,0,0},{1,0,1},{1,0,1},{1,1,1}}, // G
  {{1,0,1},{1,0,1},{1,1,1},{1,0,1},{1,0,1}}, // H
  {{1,1,1},{0,1,0},{0,1,0},{0,1,0},{1,1,1}}, // I
  {{0,1,1},{0,0,1},{0,0,1},{1,0,1},{1,1,1}}, // J
  {{1,0,1},{1,0,1},{1,1,0},{1,0,1},{1,0,1}}, // K
  {{1,0,0},{1,0,0},{1,0,0},{1,0,0},{1,1,1}}, // L
  {{1,0,1},{1,1,1},{1,0,1},{1,0,1},{1,0,1}}, // M
  {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,0,1}}, // N
  {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // O
  {{1,1,1},{1,0,1},{1,1,1},{1,0,0},{1,0,0}}, // P
  {{1,1,1},{1,0,1},{1,0,1},{1,1,1},{0,0,1}}, // Q
  {{1,1,1},{1,0,1},{1,1,0},{1,0,1},{1,0,1}}, // R
  {{0,1,1},{1,0,0},{0,1,0},{0,0,1},{1,1,0}}, // S
  {{1,1,1},{0,1,0},{0,1,0},{0,1,0},{0,1,0}}, // T
  {{1,0,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // U
  {{1,0,1},{1,0,1},{1,0,1},{1,1,0},{1,0,0}}, // V
  {{1,0,1},{1,0,1},{1,0,1},{1,1,1},{1,0,1}}, // W
  {{1,0,1},{1,0,1},{0,1,0},{1,0,1},{1,0,1}}, // X
  {{1,0,1},{1,0,1},{0,1,0},{0,1,0},{0,1,0}}, // Y
  {{1,1,1},{0,0,1},{0,1,0},{1,0,0},{1,1,1}}  // Z
};

// 8×10 digit bitmaps (0-9) stored as bool[10][8]
bool num[10][10][8] = {
  // 0
  {{0,0,1,1,1,0,0,0},{0,1,1,0,1,1,0,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {1,1,0,1,0,1,1,0},{1,1,0,1,0,1,1,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {0,1,1,0,1,1,0,0},{0,0,1,1,1,0,0,0}},
  // 1
  {{0,0,0,1,1,0,0,0},{0,0,1,1,1,0,0,0},{0,1,1,1,1,0,0,0},{0,0,0,1,1,0,0,0},
   {0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},{0,0,0,1,1,0,0,0},
   {0,0,0,1,1,0,0,0},{0,1,1,1,1,1,1,0}},
  // 2
  {{0,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,1,1,0,0},
   {0,0,0,1,1,0,0,0},{0,0,1,1,0,0,0,0},{0,1,1,0,0,0,0,0},{1,1,0,0,0,0,0,0},
   {1,1,0,0,0,1,1,0},{1,1,1,1,1,1,1,0}},
  // 3
  {{0,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},
   {0,0,1,1,1,1,0,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},
   {1,1,0,0,0,1,1,0},{0,1,1,1,1,1,0,0}},
  // 4
  {{0,0,0,0,1,1,0,0},{0,0,0,1,1,1,0,0},{0,0,1,1,1,1,0,0},{0,1,1,0,1,1,0,0},
   {1,1,0,0,1,1,0,0},{1,1,1,1,1,1,1,0},{0,0,0,0,1,1,0,0},{0,0,0,0,1,1,0,0},
   {0,0,0,0,1,1,0,0},{0,0,0,1,1,1,1,0}},
  // 5
  {{1,1,1,1,1,1,1,0},{1,1,0,0,0,0,0,0},{1,1,0,0,0,0,0,0},{1,1,0,0,0,0,0,0},
   {1,1,1,1,1,1,0,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},
   {1,1,0,0,0,1,1,0},{0,1,1,1,1,1,0,0}},
  // 6
  {{0,0,1,1,1,0,0,0},{0,1,1,0,0,0,0,0},{1,1,0,0,0,0,0,0},{1,1,0,0,0,0,0,0},
   {1,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {1,1,0,0,0,1,1,0},{0,1,1,1,1,1,0,0}},
  // 7
  {{1,1,1,1,1,1,1,0},{1,1,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},
   {0,0,0,0,1,1,0,0},{0,0,0,1,1,0,0,0},{0,0,1,1,0,0,0,0},{0,0,1,1,0,0,0,0},
   {0,0,1,1,0,0,0,0},{0,0,1,1,0,0,0,0}},
  // 8
  {{0,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {0,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {1,1,0,0,0,1,1,0},{0,1,1,1,1,1,0,0}},
  // 9
  {{0,1,1,1,1,1,0,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},{1,1,0,0,0,1,1,0},
   {0,1,1,1,1,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},{0,0,0,0,0,1,1,0},
   {0,0,0,0,1,1,0,0},{0,1,1,1,1,0,0,0}}
};

// Colon separator glyph: 2 wide × 10 tall (two dots at rows 2-3 and 6-7)
bool colon_array[] = {
  0,0,
  0,0,
  1,1,
  1,1,
  0,0,
  0,0,
  1,1,
  1,1,
  0,0,
  0,0
};

// ---- LINKED-LIST RAINBOW CYCLE ---------------------------------------------
// A circular doubly-linked list of 6 colour nodes for smooth rainbow animation

typedef struct link {
  link* next;
  link* past;
  uint16_t color;
} link;

link* current = (link*) malloc(sizeof(link) * 1);
link* head = current;
link* minus_one = NULL;

// Pre-declared array of pointers into the rainbow ring (populated by rainbow_link_setup)
link* rainbow_link_array[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

// Build the 6-node circular linked list for the rainbow colours
void rainbow_link_setup() {
  for (int i = 0; i < 5; i++) {
    current->next = (link*) malloc(sizeof(link) * 1);
    current->color = colors[i];
    minus_one = current;
    current = current->next;
    current->past = minus_one;
  }
  current->color = colors[5];
  current->next = head;
  head->past = current;

  // Populate the shortcut pointer array
  rainbow_link_array[0] = head;
  link* tmp = head;
  for (int i = 1; i < 6; i++) {
    tmp = tmp->next;
    rainbow_link_array[i] = tmp;
  }
}

// ---- BACKGROUND PATTERN FUNCTIONS -----------------------------------------

// Fill base_pixels with a diagonal rainbow pattern shifted by start (0-5)
void diagonal_rainbow(int start) {
  link* lo_current = rainbow_link_array[start % 6];
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 32; x++) {
      base_pixels[y][x] = lo_current->color;
      lo_current = lo_current->next;
    }
    lo_current = lo_current->past; // correct for row-length not being a multiple of 6
  }
}

// Fill base_pixels with a 4-column red/blue/yellow/white pattern
void RBYW() {
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 32; x++) {
      base_pixels[y][x] = (x < 8) ? RED : (x < 16) ? BLUE : (x < 24) ? YELLOW : WHITE;
    }
  }
}

// Fill base_pixels with a Duke blue/white horizontal checker pattern
void duke_checker() {
  for (int x = 0; x < 32; x++) {
    bool vert[] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1};
    for (int y = 0; y < 16; y++) {
      base_pixels[y][x] = vert[y] ? DUKE_BLUE : WHITE;
    }
  }
}

// ---- PIXEL BUFFER HELPERS --------------------------------------------------

// Draw base_pixels to the matrix (background layer)
void refill_base() {
  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 16; y++) {
      matrix.drawPixel(x, y, base_pixels[y][x]);
    }
  }
}

// ---- GLYPH DRAWING FUNCTIONS -----------------------------------------------

// Draw an 8×10 digit glyph at column start_x using top_pixels for colour.
// Fixed: original code had `x < start_x + 8` which accessed out-of-bounds
// elements of num[value][y] (only 8 columns wide) when start_x > 0.
void draw_digit(int start_x, int value) {
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 8; x++) { // 8 columns per glyph
      if (num[value][y][x]) {
        matrix.drawPixel(x + start_x, y, top_pixels[y][x + start_x]);
      }
    }
  }
}

// Draw a 3×5 small glyph (letter or digit) at (start_x, start_y) using top_pixels
void draw_small(int start_x, int start_y, int value, bool letter) {
  bool condition;
  for (int y = 0; y < 5; y++) {
    for (int x = 0; x < 3; x++) {
      condition = letter ? letters[value][y][x] : small_num[value][y][x];
      if (condition) {
        matrix.drawPixel(x + start_x, y + start_y, top_pixels[y + start_y][x + start_x]);
      }
    }
  }
}

// Draw the date in the bottom rows:
//   start_x : left edge of the first letter glyph
//   month   : pointer to a 3-element array of letter indices
//   day     : day-of-month value (1-31)
// Fixed: original code had `{day%10, day-day%10}` which produced
// the ones digit then a multiple-of-10 (e.g. 20) rather than the
// two individual digits in the correct tens/ones order.
void draw_date(int start_x, int month[3], int day) {
  // tens digit first, then ones digit
  int date[5] = {month[0], month[1], month[2], day / 10, day % 10};
  draw_small(start_x,      11, date[0], true);  // month letter 1
  draw_small(start_x + 4,  11, date[1], true);  // month letter 2
  draw_small(start_x + 8,  11, date[2], true);  // month letter 3
  draw_small(start_x + 12, 11, date[3], false); // day tens digit
  draw_small(start_x + 16, 11, date[4], false); // day ones digit
}

// Draw an arbitrary boolean bitmap at (start_x, start_y) using top_pixels
void draw_array(uint8_t start_x, uint8_t start_y, bool value[], uint8_t wide, uint8_t tall) {
  int coord = 0;
  for (int y = 0; y < tall; y++) {
    for (int x = 0; x < wide; x++) {
      if (value[coord]) {
        matrix.drawPixel(x + start_x, y + start_y, top_pixels[y + start_y][x + start_x]);
      }
      coord++;
    }
  }
}

// ---- SETUP -----------------------------------------------------------------

void setup() {
  Serial.begin(9600);

  rtc.begin();
  delay(100);
  now = rtc.now();

  // Initialise the Protomatter matrix
  ProtomatterStatus status = matrix.begin();
  matrix.setRotation(rot);

  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    for (;;); // halt on error
  }

  // Build the rainbow colour ring used by diagonal_rainbow()
  rainbow_link_setup();

  // Seed the initial background with the diagonal rainbow pattern
  diagonal_rainbow(0);

  // Set all top_pixels to white so digits appear white over the rainbow
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 32; x++) {
      top_pixels[y][x] = 65535; // white
    }
  }
}

// ---- LOOP ------------------------------------------------------------------

void loop() {
  now = rtc.now();

  // Update time_hhmm[] from current RTC reading
  if (now.twelveHour() < 10) { time_hhmm[0] = 0; time_hhmm[1] = now.twelveHour(); }
  else                       { time_hhmm[0] = 1; time_hhmm[1] = now.twelveHour() - 10; }
  time_hhmm[2] = now.minute() / 10;
  time_hhmm[3] = now.minute() % 10;

  // Advance the animated rainbow background one step
  diagonal_rainbow(scroll % 6);

  // Draw the background layer to the matrix
  refill_base();

  // Draw all four time digits over the background
  for (int d = 0; d < 4; d++) {
    draw_digit(d * 8, time_hhmm[d]);
  }

  // Draw colon separator (2 px wide, 10 px tall, centred between columns 1 and 2)
  draw_array(15, 0, colon_array, 2, 10);

  // Draw a black separator line between the time and date areas
  matrix.drawFastHLine(0, 10, 32, 0);

  // Draw the date in the bottom rows using the top_pixels colour (white)
  draw_date(10, months[now.month() - 1], now.day());

  // Push frame buffer to the physical display
  matrix.show();

  // Advance scroll for animation, wrapping at 100
  scroll++;
  if (scroll == 100) { scroll = 0; }

  delay(100);
}
