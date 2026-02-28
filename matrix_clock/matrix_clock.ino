// matrix_clock.ino
// Merged clock combining all features from lenny_clock and ella_clock.
//
// ── COMPILE-TIME CUSTOMISATION ───────────────────────────────────────────────
//   Edit the #define block below and re-upload to change the default look.
//
// ── RUNTIME CUSTOMISATION ────────────────────────────────────────────────────
//   Button A (BTN_PALETTE_PIN, default A0): short press → next palette
//   Button B (BTN_PATTERN_PIN, default A1): short press → next pattern
//   Both buttons held for SET_HOLD_MS (default 5 s) → enter time/date set mode:
//     • Button A increments the highlighted field (hour → minute → month → day)
//     • Button B advances to the next field; after day, saves and exits
//     • Hold both buttons again for 5 s to cancel without saving
//   Both buttons are active-LOW.  Wire one leg to the pin, other leg to GND.
//   The sketch uses INPUT_PULLUP, so no external resistors are needed.
//
// ── FEATURES ─────────────────────────────────────────────────────────────────
//   • 19 named colour palettes (cycle at runtime with Button A)
//   • 14 animated/static background patterns (cycle at runtime with Button B)
//   • Time/date set mode: hold both buttons 5 s, A increments, B advances field
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

// Starting palette  (1–19).  Cycle at runtime with Button A.
#define DEFAULT_PALETTE  1

// Starting pattern  (0–13).   Cycle at runtime with Button B.
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

// How long (in milliseconds) both buttons must be held simultaneously to enter
// or cancel time/date set mode.  5000 ms = 5 seconds.
#define SET_HOLD_MS      5000

// How often (in milliseconds) the scheduler polls the button pins.
// 10 ms gives responsive feel while staying well inside the debounce window.
#define BTN_POLL_MS      10

// Target frame period in milliseconds.  50 ms = 20 fps — smooth for a clock
// and slow enough that the scroll counter advances at a humanly-visible rate.
// Rendering is skipped when less than FRAME_MS has elapsed since the last
// rendered frame, so buttons and RTC reads remain responsive at all times.
#define FRAME_MS         50

// How many rendered frames to skip between each one-step advance of the
// animation scroll counter.  scroll advances every FRAME_MS*SCROLL_DIVIDER ms.
//   4 → every 200 ms; a 6-colour palette completes one full cycle in ~1.2 s.
// Increasing this slows the animation; 1 makes it advance every frame (strobe).
#define SCROLL_DIVIDER   4

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
Task update_digits_task(1000, -1, &access_rtc);    // RTC read once per second
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

// ============================================================
//  TIME/DATE SET MODE STATE
// ============================================================

bool          set_mode        = false; // true while in time/date-set UI
int           set_field       = 0;    // active field: 0=hour 1=min 2=month 3=day
int           set_h           = 0;    // working hour   (0–23, 24-hour)
int           set_m           = 0;    // working minute (0–59)
int           set_mo          = 1;    // working month  (1–12)
int           set_d           = 1;    // working day    (1–31)
int           set_yr          = 2024; // captured year (used on save to avoid midnight year-edge case)
unsigned long both_held_since = 0;    // millis() when both buttons went LOW together

// ============================================================
//  SET MODE HELPERS
// ============================================================

// Return the maximum valid day for a given month and year (leap-year aware).
static int days_in_month(int month, int year) {
  static const int dom[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  if (month == 2) {
    bool leap = (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
    return leap ? 29 : 28;
  }
  return dom[month];
}

// Snapshot the current RTC values, disable the automatic RTC-read task so
// the display is frozen for editing, and switch to set mode.
void enter_set_mode() {
  DateTime t = rtc.now();
  set_yr = t.year();    // captured now to avoid a year-edge at midnight
  set_h  = t.hour();    // 0–23 (24-hour)
  set_m  = t.minute();
  set_mo = t.month();
  set_d  = t.day();
  set_field = 0;
  set_mode  = true;
  // Set debounce timestamps so the first press in set mode is always valid.
  last_palette_press = millis() - BTN_DEBOUNCE_MS - 1;
  last_pattern_press = millis() - BTN_DEBOUNCE_MS - 1;
  update_digits_task.disable();
}

// Exit set mode.  If save=true, write the edited values to the RTC; always
// re-enable the RTC-read task and refresh digits[] from the (possibly updated) time.
void exit_set_mode(bool save) {
  if (save) {
    rtc.adjust(DateTime(set_yr, set_mo, set_d, set_h, set_m, 0));
  }
  set_mode = false;
  last_palette_press = millis() - BTN_DEBOUNCE_MS - 1;
  last_pattern_press = millis() - BTN_DEBOUNCE_MS - 1;
  update_digits_task.enable();
  access_rtc();  // refresh digits[] / date_array[] immediately
}

// Increment the active field value with wrap-around.  When the month changes,
// the day is clamped to the new month's maximum so an invalid date is never
// stored (e.g. January 31 → switching to February clamps the day to 28/29).
void increment_set_field() {
  switch (set_field) {
    case 0: set_h  = (set_h  +  1) % 24; break;
    case 1: set_m  = (set_m  +  1) % 60; break;
    case 2:
      set_mo = (set_mo % 12) + 1;   // 1→2→…→12→1
      // Clamp day to the new month's actual maximum (leap-year aware).
      { int max_d = days_in_month(set_mo, set_yr);
        if (set_d > max_d) set_d = max_d; }
      break;
    case 3: set_d = (set_d % days_in_month(set_mo, set_yr)) + 1; break;
  }
}

// Advance to the next field.  After the last field (day), save and exit.
void advance_set_field() {
  if (++set_field > 3) exit_set_mode(true);
}

// ============================================================
//  BUTTON HANDLING
// ============================================================

// Poll both buttons; cycle palette/pattern normally, or drive set mode when active.
void check_buttons() {
  unsigned long now_ms = millis();
  bool a_low = digitalRead(BTN_PALETTE_PIN) == LOW;
  bool b_low = digitalRead(BTN_PATTERN_PIN) == LOW;

  // ── Both-button hold: enter set mode (normal) or cancel set mode ─────────
  if (a_low && b_low) {
    if (both_held_since == 0) both_held_since = now_ms;
    if ((now_ms - both_held_since) >= SET_HOLD_MS) {
      both_held_since = 0;
      if (!set_mode) enter_set_mode();
      else           exit_set_mode(false);  // cancel without saving
    }
    return;  // don't process individual presses while both are held
  }
  both_held_since = 0;

  // ── Set mode: A increments active field, B advances to next field ─────────
  if (set_mode) {
    if (a_low && (now_ms - last_palette_press) > BTN_DEBOUNCE_MS) {
      last_palette_press = now_ms;
      increment_set_field();
    }
    if (b_low && (now_ms - last_pattern_press) > BTN_DEBOUNCE_MS) {
      last_pattern_press = now_ms;
      advance_set_field();  // saves and exits after the day field
    }
    return;
  }

  // ── Normal mode ──────────────────────────────────────────────────────────
  if (a_low && (now_ms - last_palette_press) > BTN_DEBOUNCE_MS) {
    last_palette_press = now_ms;
    change_pal_helper();   // advance current_palette
    change_palette();      // load the new palette into palette[] and ink_color[]
  }

  if (b_low && (now_ms - last_pattern_press) > BTN_DEBOUNCE_MS) {
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

// Read the RTC once per second and update digits[] and date_array[].
//
// Two guard layers protect against corrupted reads:
//
//   1. Double-read: two back-to-back rtc.now() calls must agree on hour and
//      minute.  The DS3231's BCD registers are not read atomically; a counter
//      increment that occurs mid-read, or I2C timing disturbed by the matrix
//      DMA interrupt, can produce a snapshot where hour and minute belong to
//      different seconds.  Sampling twice in quick succession (microseconds
//      apart, far less than one RTC tick) and discarding if they disagree
//      virtually guarantees a coherent pair.
//
//   2. Range guard: minute > 59 catches the specific BCD rollover value (60)
//      that the DS3231 briefly shows at the top of each hour.
void access_rtc() {
  DateTime a = rtc.now();
  DateTime b = rtc.now();
  if (a.hour() != b.hour() || a.minute() != b.minute()) return;

  now = a;
  int m  = now.minute();
  int hr = now.twelveHour();  // 1–12, never 0

  if (m > 59) return;

#if ENABLE_DST
  if (check_dst()) hr = (hr % 12) + 1;
#endif

  digits[0] = (hr < 10) ? 0 : 1;
  digits[1] = (hr < 10) ? hr : hr - 10;
  digits[2] = m / 10;
  digits[3] = m % 10;

  for (int i = 0; i < 3; i++)
    date_array[i] = months[now.month() - 1][i];
  date_array[3] = now.day() / 10;
  date_array[4] = now.day() % 10;
}

// ============================================================
//  DISPLAY: TIME DIGITS
// ============================================================

// Draw the four time-digit glyphs (foreground / lit pixels only) in ink_color[].
//
// The background behind unlit pixels is whatever the pattern painted.
// fillScreen(0) at the top of every frame guarantees a clean slate, so unlit
// pixels that the pattern does not reach default to black — no stale content.
//
// Layout when colon=true and the leading hour digit is 0 (hours 1–9):
//   cols  0– 3 : blank
//   cols  4–11 : hour digit       (digits[1])
//   cols 12–15 : colon dots       (ink_color[1])
//   cols 16–23 : tens-of-minutes  (digits[2])
//   cols 24–31 : ones-of-minutes  (digits[3])
//
// All other cases use the simple 4-digit layout at x-offsets 0, 8, 16, 24.
void display_time(bool colon) {
  if (!colon || digits[0]) {
    // Normal 4-digit layout (colon disabled, or two-digit hour 10–12).
    for (int dig = 0; dig < 4; dig++) {
      for (int i = 0; i < 80; i++) {
        if (num[digits[dig]][i])
          matrix.drawPixel(trans_x(i) + dig * 8, trans_y(i), ink_color[dig]);
      }
    }
    return;
  }

  // Single-digit hour (1–9) with colon: shift everything right by 4 pixels.
  for (int i = 0; i < 80; i++) {
    if (num[digits[1]][i])
      matrix.drawPixel(trans_x(i) + 4, trans_y(i), ink_color[1]);
  }
  matrix.fillRect(13, 2, 2, 2, ink_color[1]);  // upper colon dot
  matrix.fillRect(13, 6, 2, 2, ink_color[1]);  // lower colon dot
  for (int i = 0; i < 80; i++) {
    if (num[digits[2]][i])
      matrix.drawPixel(trans_x(i) + 16, trans_y(i), ink_color[2]);
    if (num[digits[3]][i])
      matrix.drawPixel(trans_x(i) + 24, trans_y(i), ink_color[3]);
  }
}

// ============================================================
//  DISPLAY: DATE
// ============================================================

// Draw the 3-char month abbreviation and 2-digit day in the bottom rows
// using 3×5 pixel small glyphs.
// Each glyph gets a 3×5 black backdrop drawn first so the text is readable
// against any background pattern colour.
void display_date(uint16_t color) {
  // Month abbreviation: three letter glyphs starting at x=10, y=11
  for (int place = 0; place < 3; place++) {
    matrix.fillRect(10 + place * 4, 11, 3, 5, 0);  // black backdrop per glyph
    for (int i = 0; i < 15; i++) {
      if (letters[date_array[place]][i]) {
        matrix.drawPixel(10 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
  // Day number: two digit glyphs after the month abbreviation
  for (int place = 3; place < 5; place++) {
    matrix.fillRect(11 + place * 4, 11, 3, 5, 0);  // black backdrop per glyph
    for (int i = 0; i < 15; i++) {
      if (small_num[date_array[place]][i]) {
        matrix.drawPixel(11 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
}

// ============================================================
//  DISPLAY: SET MODE OVERLAY
// ============================================================

// Draw the time/date-set UI.  Called from loop() instead of the normal
// display_time() / display_date() pair while set_mode is true.
//
// The active field blinks (500 ms on / 300 ms off).  All other fields are
// shown steady in the current ink colour.  The active field is drawn bright
// white so it stands out clearly against any background pattern.
//
// Time is shown in 24-hour format (0–23) as four digits in the top rows,
// matching the big-glyph layout used by display_time().
// Date is shown as a 3-letter month abbreviation + 2-digit day at the bottom,
// identical to display_date().
//
// Two white pixels at x=0–1, y=10 (the separator row) act as a small always-on
// indicator that set mode is active.
void display_set_time() {
  bool blink_on = (millis() % 800) < 500;
  uint16_t ink  = ink_color[0];

  // ── Time (24-hour, always 4-digit) in rows 0–9 ───────────────────────────
  int d[4] = { set_h / 10, set_h % 10, set_m / 10, set_m % 10 };
  for (int dig = 0; dig < 4; dig++) {
    bool active = (set_field == 0 && dig < 2) || (set_field == 1 && dig >= 2);
    if (active && !blink_on) continue;          // hide on the "off" half-cycle
    uint16_t c = active ? WHITE : ink;          // bright white when this field is active
    for (int i = 0; i < 80; i++) {
      if (num[d[dig]][i])
        matrix.drawPixel(trans_x(i) + dig * 8, trans_y(i), c);
    }
  }

  // ── Separator ────────────────────────────────────────────────────────────
  matrix.drawFastHLine(0, 10, 32, 0);

  // ── Set-mode indicator: two bright pixels at left of separator row ────────
  matrix.drawPixel(0, 10, WHITE);
  matrix.drawPixel(1, 10, WHITE);

  // ── Date: month abbreviation (3 letters) in rows 11–15 ───────────────────
  for (int place = 0; place < 3; place++) {
    bool active = (set_field == 2);
    matrix.fillRect(10 + place * 4, 11, 3, 5, 0);
    if (!(active && !blink_on)) {
      uint16_t c = active ? WHITE : ink;
      for (int i = 0; i < 15; i++) {
        if (letters[months[set_mo - 1][place]][i])
          matrix.drawPixel(10 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, c);
      }
    }
  }

  // ── Date: day-of-month (2 digits) in rows 11–15 ──────────────────────────
  int day_d[2] = { set_d / 10, set_d % 10 };
  for (int place = 0; place < 2; place++) {
    bool active = (set_field == 3);
    matrix.fillRect(11 + (place + 3) * 4, 11, 3, 5, 0);
    if (!(active && !blink_on)) {
      uint16_t c = active ? WHITE : ink;
      for (int i = 0; i < 15; i++) {
        if (small_num[day_d[place]][i])
          matrix.drawPixel(11 + trans_x_smol(i) + (place + 3) * 4, trans_y_smol(i) + 11, c);
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
  // Run the scheduler every iteration so buttons (10 ms) and RTC (1000 ms)
  // are serviced at their own cadence, independent of the render rate.
  face_scheduler.execute();

  // Enforce the target frame rate.
  static unsigned long last_frame = 0;
  unsigned long now_ms = millis();
  if ((now_ms - last_frame) < FRAME_MS) return;
  last_frame = now_ms;

  // Advance the animation counter every SCROLL_DIVIDER frames so the
  // background drifts at a pleasant pace rather than strobing once per frame.
  static uint8_t scroll_div = 0;
  if (++scroll_div >= SCROLL_DIVIDER) {
    scroll_div = 0;
    scroll = (scroll + 1) % 600;  // 600 = LCM(4,6)×50 — no colour jump at wrap
  }

  // ── Frame rendering ──────────────────────────────────────────────────────
  // Protomatter double-buffering SWAPS the two frame buffers on show().
  // After the swap the new back buffer contains the former front buffer's
  // pixels — whatever was drawn TWO frames ago, not a clean slate.
  // Without an explicit clear, any pixel not repainted this frame retains
  // its stale colour, causing ghost digits and date characters to oscillate
  // at 10 Hz (half the frame rate), which looks like "jumping."
  matrix.fillScreen(0);                              // 1. clean slate

  if (draw_current_pattern) draw_current_pattern();  // 2. background pattern

  if (set_mode) {
    display_set_time();                              // 3a. set-mode overlay
  } else {
    display_time(ENABLE_COLON);                      // 3b. time digits
    matrix.drawFastHLine(0, 10, 32, 0);             // 4. separator row
    display_date(WHITE);                             // 5. date
  }

  matrix.show();                                     // 6. swap to display
}
