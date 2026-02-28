// matrix_clock.ino
// Merged clock combining all features from lenny_clock and ella_clock.
//
// ── COMPILE-TIME CUSTOMISATION ───────────────────────────────────────────────
//   Edit the #define block below and re-upload to change the default look.
//
// ── RUNTIME CUSTOMISATION ────────────────────────────────────────────────────
//   Button A (BTN_PALETTE_PIN, default A0): short press → next palette
//   Button B (BTN_PATTERN_PIN, default A1): short press → next pattern
//   Both buttons are active-LOW.  Wire one leg to the pin, other leg to GND.
//   The sketch uses INPUT_PULLUP, so no external resistors are needed.
//
// ── FEATURES ─────────────────────────────────────────────────────────────────
//   • 11 named colour palettes (cycle at runtime with Button A)
//   • 8 animated/static background patterns (cycle at runtime with Button B)
//   • Per-digit ink colours, driven by the active palette
//   • Optional colon separator with single-digit-hour shift
//   • Optional North-American DST detection (adjusts the displayed hour only)
//   • Adjustable display brightness
//   • TaskScheduler for non-blocking RTC reads, pattern animation,
//     and button polling (BTN_POLL_MS interval, default 10 ms)
//   • Special overlays: blaze_it(), birthday()
// ─────────────────────────────────────────────────────────────────────────────

// ============================================================
//  COMPILE-TIME CONFIGURATION — edit these before uploading
// ============================================================

// Starting palette  (1–11).  Cycle at runtime with Button A.
#define DEFAULT_PALETTE  1

// Starting pattern  (0–7).   Cycle at runtime with Button B.
#define DEFAULT_PATTERN  0

// Draw a colon between the hour and minute digits.
// When the leading hour digit is 0 the display shifts right to make room.
// 1 = enabled, 0 = disabled.
#define ENABLE_COLON     1

// Detect North-American DST and add one hour to the displayed time while DST
// is in effect (the RTC itself is never modified).
// 1 = enabled, 0 = disabled.
#define ENABLE_DST       0

// Overall brightness, 0.0 (off) – 1.0 (full).
// Values below 1.0 reduce colour saturation as well as brightness.
#define BRIGHTNESS       1.0

// Button pin assignments (active-LOW with INPUT_PULLUP).
// Change these to match wherever you wire your buttons.
// NOTE: A0 = D14 = clockPin and A1 = D15 = latchPin on Metro M4, so those
//       pins cannot be used for buttons.  D2 and D3 are safe choices.
#define BTN_PALETTE_PIN  2
#define BTN_PATTERN_PIN  3

// Minimum milliseconds between button presses (debounce).
#define BTN_DEBOUNCE_MS  200

// How often (in milliseconds) the scheduler polls the button pins.
// 10 ms gives responsive feel while staying well inside the debounce window.
#define BTN_POLL_MS      10

// ============================================================
//  LIBRARIES
// ============================================================

#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "font_array.h"
#include <TaskScheduler.h>
#include <TaskSchedulerDeclarations.h>
#include <TaskSchedulerSleepMethods.h>

// ============================================================
//  COLOUR ALIASES  (index into colors[] defined below)
// ============================================================

#define RED       colors[0]
#define ORANGE    colors[1]
#define YELLOW    colors[2]
#define GREEN     colors[3]
#define BLUE      colors[4]
#define PURPLE    colors[5]
#define BLACK     colors[6]
#define WHITE     colors[7]
#define GRAY      colors[8]
#define DUKE_BLUE colors[9]
#define CYAN      colors[10]
#define MAGENTA   colors[11]
#define PURE_RED   matrix.color565(255,0,0)
#define PURE_GREEN matrix.color565(0,255,0)
#define PURE_BLUE  matrix.color565(0,0,255)

// ============================================================
//  HARDWARE PINS
// ============================================================

// Metro M4 (default) — comment out and uncomment the RP2040 block below if needed
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;
int rot = 2; // display rotation (0/1/2/3)

// Feather RP2040 — uncomment and comment the block above if using that board:
// uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
// uint8_t addrPins[] = {25, 24, 29, 28};
// uint8_t clockPin   = 13;
// uint8_t latchPin   = 1;
// uint8_t oePin      = 0;
// int rot = 0;

// ============================================================
//  MATRIX AND RTC OBJECTS
// ============================================================

RTC_DS3231 rtc;
DateTime now;

// 32 px wide, 4-bit colour depth, single chain, 3 address pins (height=16 inferred)
Adafruit_Protomatter matrix(
  32, 4,
  1, rgbPins,
  3, addrPins,
  clockPin, latchPin, oePin,
  true  // double-buffering: eliminates flicker and errant-line tearing
);

// ============================================================
//  NAMED COLOUR TABLE
// ============================================================

uint16_t colors[] = {
  matrix.color565(255, 0,   0),    //  0  Red
  matrix.color565(253, 152, 0),    //  1  Orange
  matrix.color565(255, 255, 0),    //  2  Yellow
  matrix.color565(51,  254, 0),    //  3  Green
  matrix.color565(0,   151, 253),  //  4  Blue
  matrix.color565(102, 51,  253),  //  5  Purple
  0,                               //  6  Black
  65535,                           //  7  White
  matrix.color565(72,  72,  72),   //  8  Gray
  matrix.color565(28,  33,  168),  //  9  Duke Blue
  matrix.color565(0,   255, 255),  // 10  Cyan
  matrix.color565(255, 0,   255),  // 11  Magenta
};

// ============================================================
//  ACTIVE PALETTE & PATTERN STATE
// ============================================================

uint16_t palette[6];                           // up to 6 colours in the active background palette
uint16_t ink_color[4] = {WHITE, WHITE, WHITE, WHITE}; // per-digit foreground colour
int palette_size    = 6;
int current_palette = DEFAULT_PALETTE;
int current_pattern = DEFAULT_PATTERN;
int scroll          = 0;                       // animation counter

// ============================================================
//  DIGIT AND DATE STATE
// ============================================================

// digits[0..3] = [H-tens, H-ones, M-tens, M-ones] for the display glyphs
int digits[]     = {1, 0, 2, 3};

// date_array[0..2] = letter indices for 3-char month abbreviation
// date_array[3]    = tens digit of day-of-month
// date_array[4]    = ones digit of day-of-month
int date_array[] = {0, 0, 0, 0, 0};

// ============================================================
//  TASK SCHEDULER
// ============================================================

// Forward declarations needed because tasks reference functions defined below.
void access_rtc();
void check_buttons();

Scheduler face_scheduler;
Task update_digits_task(50, -1, &access_rtc);      // RTC read every 50 ms
Task btn_task(BTN_POLL_MS, -1, &check_buttons);    // button poll every BTN_POLL_MS

// Function pointer to the active background pattern drawing function.
// Set by switch_pattern() and called directly in loop() every frame so
// the back buffer is always fully repainted before digits are overlaid.
typedef void (*PatternFn)();
PatternFn draw_current_pattern = nullptr;

// face_task_list.h defines all pattern/palette helpers and must be included
// here so it can reference the variables above.
#include "face_task_list.h"

// ============================================================
//  BUTTON HANDLING
// ============================================================

unsigned long last_palette_press = 0;
unsigned long last_pattern_press = 0;

// Poll both buttons; cycle palette or pattern on a debounced falling edge.
void check_buttons() {
  unsigned long now_ms = millis();

  if (digitalRead(BTN_PALETTE_PIN) == LOW &&
      (now_ms - last_palette_press) > BTN_DEBOUNCE_MS) {
    last_palette_press = now_ms;
    change_pal_helper();   // advance current_palette
    change_palette();      // load the new palette into palette[] and ink_color[]
  }

  if (digitalRead(BTN_PATTERN_PIN) == LOW &&
      (now_ms - last_pattern_press) > BTN_DEBOUNCE_MS) {
    last_pattern_press = now_ms;
    change_pat_helper();              // advance current_pattern
    switch_pattern(current_pattern);  // update draw_current_pattern function pointer
  }
}

// ============================================================
//  BRIGHTNESS CONTROL  (from lenny_clock)
// ============================================================

// Scale all named colours by br (0.0 = off, 1.0 = full brightness).
// Call update_brightness() before change_palette() so palette colours are
// derived from the already-scaled values.
void update_brightness(float br) {
  colors[0]  = matrix.color565(255*br, 0,       0);
  colors[1]  = matrix.color565(253*br, 152*br,  0);
  colors[2]  = matrix.color565(255*br, 255*br,  0);
  colors[3]  = matrix.color565(51*br,  254*br,  0);
  colors[4]  = matrix.color565(0,      151*br,  253*br);
  colors[5]  = matrix.color565(102*br, 51*br,   253*br);
  // colors[6] is always black (0), no scale needed
  colors[7]  = matrix.color565(255*br, 255*br,  255*br);
  colors[8]  = matrix.color565(72*br,  72*br,   72*br);
  colors[9]  = matrix.color565(28*br,  33*br,   168*br);
  colors[10] = matrix.color565(0,      255*br,  255*br);
  colors[11] = matrix.color565(255*br, 0,       255*br);
}

// ============================================================
//  DST HELPERS  (from ella_clock, conditionally compiled)
// ============================================================

#if ENABLE_DST
// Return the DateTime of the second Sunday in March (DST start, North America).
DateTime calc_dst_start() {
  DateTime ref = rtc.now();
  DateTime this_day;
  int day_of_month = 1;
  for (int num_sunday = 0; num_sunday < 2; ) {
    this_day = DateTime(ref.year(), 3, day_of_month);
    if (!this_day.dayOfTheWeek()) { num_sunday++; }
    day_of_month++;
  }
  return this_day;
}

// Return the DateTime of the first Sunday in November (DST end, North America).
DateTime calc_dst_end() {
  DateTime ref = rtc.now();
  int day_of_month = 1;
  DateTime this_day;
  for (int num_sunday = 0; num_sunday < 1; ) {
    this_day = DateTime(ref.year(), 11, day_of_month);
    if (!this_day.dayOfTheWeek()) { num_sunday++; }
    day_of_month++;
  }
  return this_day;
}

// Return true when the current time falls within North-American DST.
bool check_dst() {
  DateTime t = rtc.now();
  return (calc_dst_start() <= t) && (t <= calc_dst_end());
}
#endif // ENABLE_DST

// ============================================================
//  COORDINATE HELPERS
// ============================================================

int trans_x(int pos)      { return pos % 8; } // flat index → glyph column (0-7)
int trans_y(int pos)      { return pos / 8; } // flat index → glyph row   (0-9)
int trans_x_smol(int pos) { return pos % 3; } // flat index → small glyph column (0-2)
int trans_y_smol(int pos) { return pos / 3; } // flat index → small glyph row    (0-4)

// ============================================================
//  RTC TASK CALLBACK
// ============================================================

// Reads the RTC, populates digits[] (12-hour time) and date_array[],
// and applies a +1-hour DST offset to the displayed hour when ENABLE_DST=true.
void access_rtc() {
  now = rtc.now();

  // Guard against DS3231 BCD rollover glitch that can momentarily return
  // minute=60 at the top of each hour.  Skip the entire update in that case
  // so digits[] always hold a valid 0-59 minute value.
  int m = now.minute();
  if (m > 59) return;

  int hour = now.twelveHour(); // 1–12

#if ENABLE_DST
  if (check_dst()) {
    // Advance the displayed hour by one; wrap 12 → 1.
    hour = (hour % 12) + 1;
  }
#endif

  if (hour < 10) { digits[0] = 0; digits[1] = hour; }
  else           { digits[0] = 1; digits[1] = hour - 10; }

  digits[2] = m / 10;
  digits[3] = m % 10;

  for (int letter = 0; letter < 3; letter++) {
    date_array[letter] = months[now.month() - 1][letter];
  }
  date_array[3] = now.day() / 10;
  date_array[4] = now.day() % 10;
}

// ============================================================
//  DISPLAY: TIME DIGITS
// ============================================================

// Draw the four time-digit glyphs onto the matrix.
//
//  colon — when true AND the leading hour digit is 0, shift the layout
//           right and draw two colon dots between the hour and minutes.
//  bg    — when false, draw the lit (foreground) pixels in ink_color[];
//           when true,  draw the unlit (background) pixels in ink_color[]
//           (used by background pattern functions to fill around digits).
void display_time(bool colon, bool bg) {
  if (!colon || digits[0]) {
    // Normal four-digit layout (or colon disabled).
    for (int dig = 0; dig < 4; dig++) {
      for (int i = 0; i < 80; i++) {
        if ((num[digits[dig]][i] && !bg) || (!num[digits[dig]][i] && bg)) {
          matrix.drawPixel(trans_x(i) + dig * 8, trans_y(i), ink_color[dig]);
        }
      }
    }
    return;
  }

  // Single-digit hour with colon — shifted layout:
  //   cols  0-3  : blank / leading space
  //   cols  4-11 : hour digit
  //   cols 12-15 : colon dots
  //   cols 16-23 : tens-of-minutes digit
  //   cols 24-31 : ones-of-minutes digit

  if (bg) {
    // Background pixels (the "holes" around each glyph)
    matrix.fillRect(0, 0, 4, 10, 0);
    for (int i = 0; i < 80; i++) {
      if (!num[digits[1]][i]) {
        matrix.drawPixel(trans_x(i) + 4, trans_y(i), ink_color[1]);
      }
    }
    // Clear colon column backgrounds
    matrix.fillRect(12, 0, 4, 2, 0);
    matrix.fillRect(12, 8, 4, 2, 0);
    matrix.fillRect(12, 0, 1, 10, 0);
    matrix.fillRect(12, 4, 4, 2, 0);
    matrix.fillRect(15, 0, 1, 10, 0);
    for (int i = 0; i < 80; i++) {
      if (!num[digits[2]][i]) {
        matrix.drawPixel(trans_x(i) + 16, trans_y(i), ink_color[2]);
      }
      if (!num[digits[3]][i]) {
        matrix.drawPixel(trans_x(i) + 24, trans_y(i), ink_color[3]);
      }
    }
    return;
  }

  // Foreground pixels (the lit segments and colon dots)
  for (int i = 0; i < 80; i++) {
    if (num[digits[1]][i]) {
      matrix.drawPixel(trans_x(i) + 4, trans_y(i), ink_color[1]);
    }
  }
  matrix.fillRect(13, 2, 2, 2, ink_color[1]); // upper colon dot
  matrix.fillRect(13, 6, 2, 2, ink_color[1]); // lower colon dot
  for (int i = 0; i < 80; i++) {
    if (num[digits[2]][i]) {
      matrix.drawPixel(trans_x(i) + 16, trans_y(i), ink_color[2]);
    }
    if (num[digits[3]][i]) {
      matrix.drawPixel(trans_x(i) + 24, trans_y(i), ink_color[3]);
    }
  }
}

// ============================================================
//  DISPLAY: DATE
// ============================================================

// Draw the 3-char month abbreviation and 2-digit day in the bottom rows
// using 3×5 pixel small glyphs.
void display_date(uint16_t color) {
  // Month abbreviation: three letter glyphs starting at x=10, y=11
  for (int place = 0; place < 3; place++) {
    for (int i = 0; i < 15; i++) {
      if (letters[date_array[place]][i]) {
        matrix.drawPixel(10 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
  // Day number: two digit glyphs after the month abbreviation
  for (int place = 3; place < 5; place++) {
    for (int i = 0; i < 15; i++) {
      if (small_num[date_array[place]][i]) {
        matrix.drawPixel(11 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
}

// ============================================================
//  SPECIAL TEXT OVERLAYS  (from lenny_clock)
// ============================================================

// Letter indices into the letters[] glyph table (A=0, B=1, …, Z=25)
int blaze_num[]    = {1, 11, 0, 25, 4, 8, 19};   // B L A Z E I T
int birthday_num[] = {1, 8, 17, 19, 7, 3, 0, 24}; // B I R T H D A Y

// Draw "BLAZE IT" in red across the bottom five rows.
// Call instead of display_date() on April 20 if desired.
void blaze_it() {
  for (int place = 0; place < 5; place++) {
    for (int i = 0; i < 15; i++) {
      if (letters[blaze_num[place]][i]) {
        matrix.drawPixel(1 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, RED);
      }
    }
  }
  for (int place = 5; place < 7; place++) {
    for (int i = 0; i < 15; i++) {
      if (letters[blaze_num[place]][i]) {
        matrix.drawPixel(2 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, RED);
      }
    }
  }
}

// Draw "BIRTHDAY" in the bottom rows by blanking the non-letter pixels.
// Call instead of display_date() to replace the date with a birthday message.
void birthday() {
  for (int place = 0; place < 8; place++) {
    for (int i = 0; i < 15; i++) {
      if (!letters[birthday_num[place]][i]) {
        matrix.drawPixel(1 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, 0);
      }
    }
    matrix.drawFastVLine(place * 4, 11, 5, 0);
  }
}

// ============================================================
//  SETUP
// ============================================================

void setup(void) {
  Serial.begin(9600);
  rtc.begin();

  // Configure buttons as active-LOW inputs (internal pull-up, no resistors needed)
  pinMode(BTN_PALETTE_PIN, INPUT_PULLUP);
  pinMode(BTN_PATTERN_PIN, INPUT_PULLUP);

  delay(100);
  now = rtc.now();

  // Initialise the Protomatter matrix
  ProtomatterStatus status = matrix.begin();
  matrix.setRotation(rot);

  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    // Halt if the matrix could not be initialised.
    for (;;);
  }

  // Apply compile-time brightness (rebuilds colors[] before change_palette()).
  update_brightness(BRIGHTNESS);

  // Seed the RTC state so digits[] and date_array[] are valid before first draw.
  access_rtc();

  // Load the compile-time default palette and pattern.
  current_palette = DEFAULT_PALETTE;
  change_palette();
  current_pattern = DEFAULT_PATTERN;
  switch_pattern(DEFAULT_PATTERN);

  // Register and enable the scheduler tasks.
  face_scheduler.addTask(update_digits_task);
  face_scheduler.addTask(btn_task);
  update_digits_task.enable();
  btn_task.enable();

  matrix.show();
}

// ============================================================
//  LOOP
// ============================================================

void loop() {
  // Run scheduled tasks: button poll and RTC refresh.
  face_scheduler.execute();

  // Draw the background pattern every frame so the back buffer is always
  // fully repainted before digits are overlaid.  This prevents stale pixels
  // (ghost digits, leftover palette colours) from accumulating between frames.
  if (draw_current_pattern) draw_current_pattern();

  // Overlay the time digits in ink_color[] on top of the background.
  display_time(ENABLE_COLON, false);

  // Thin black separator line between the time and date rows.
  matrix.drawFastHLine(0, 10, 32, 0);

  // Date in neutral grey.  Replace this call with blaze_it() or birthday()
  // if you want a special overlay on a particular day.
  display_date(matrix.color565(128, 128, 128));

  // Push the frame buffer to the physical LEDs.
  matrix.show();

  // Advance the scroll counter used by animated patterns.
  scroll++;
  if (scroll == 100) { scroll = 0; }
}
