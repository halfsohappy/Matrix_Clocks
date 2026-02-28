// ella_clock.ino
// An RGB LED matrix clock with animated background patterns, a palette/pattern
// system, DST detection, and a TaskScheduler for non-blocking operation.
//
// Hardware:
//   - 32×16 RGB LED matrix driven by Adafruit Protomatter
//   - DS3231 real-time clock module
//   - Metro/Feather M4 (or RP2040 Feather) microcontroller
//
// Display layout:
//   Rows  0-9  : four 8×10 digit glyphs showing HH:MM in 12-hour format
//   Row   10   : black separator line
//   Rows 11-15 : 3-char month abbreviation + 2-digit day in small 3×5 glyphs

// Libraries
#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "font_array.h"
#include <TaskScheduler.h>
#include <TaskSchedulerDeclarations.h>
#include <TaskSchedulerSleepMethods.h>

// Target frame period in milliseconds.  50 ms = 20 fps.
// See matrix_clock for full rationale.
#define FRAME_MS  50

// Rendered frames to skip between each one-step advance of the animation
// scroll counter.  At 20 fps, scroll advances every FRAME_MS*SCROLL_DIVIDER ms.
// 4 → every 200 ms; a 6-colour palette cycles fully in ~1.2 s.
#define SCROLL_DIVIDER  4

// Colour aliases that index into the colors[] array defined below
#define RED colors[0]
#define ORANGE colors[1]
#define YELLOW colors[2]
#define GREEN colors[3]
#define BLUE colors[4]
#define PURPLE colors[5]
#define BLACK colors[6]
#define WHITE colors[7]
#define GRAY colors[8]
#define DUKE_BLUE colors[9]
#define CYAN colors[10]
#define MAGENTA colors[11]
#define PURE_RED   matrix.color565(255,0,0)
#define PURE_GREEN matrix.color565(0,255,0)
#define PURE_BLUE  matrix.color565(0,0,255)

// MATRIX PINS (Metro/Feather M4 or compatible):
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;
int rot = 2; // display rotation (0/1/2/3)

// RP2040 Feather pins (uncomment and comment the block above if using that board):
// uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
// uint8_t addrPins[] = {25, 24, 29, 28};
// uint8_t clockPin   = 13;
// uint8_t latchPin   = 1;
// uint8_t oePin      = 0;
// int rot = 0;

// Real-time clock and current timestamp
RTC_DS3231 rtc;
DateTime now;

// digits[0..3] = [H1, H2, M1, M2] for the four display glyphs
int digits[] = {1, 0, 2, 3};
// date_array[0..2] = letter indices for 3-char month abbreviation
// date_array[3]    = tens digit of day-of-month
// date_array[4]    = ones digit of day-of-month
int date_array[] = {0, 0, 0, 0, 0};

// Adafruit Protomatter matrix: 32 px wide, 4-bit colour depth, single chain,
// 3 address pins (height inferred as 16), double-buffering enabled to
// eliminate flickering and errant-line tearing
Adafruit_Protomatter matrix(
  32,           // Width of matrix in pixels
  4,            // Bit depth, 1-6
  1, rgbPins,   // # of matrix chains, RGB pins
  3, addrPins,  // # of address pins, address pin array
  clockPin, latchPin, oePin,
  true          // Double-buffering: eliminates flicker and errant-line tearing
);

// Named colour palette
uint16_t colors[] = {
  matrix.color565(255, 0, 0),       // 0 Red
  matrix.color565(253, 152, 0),     // 1 Orange
  matrix.color565(255, 255, 0),     // 2 Yellow
  matrix.color565(51, 254, 0),      // 3 Green
  matrix.color565(0, 151, 253),     // 4 Blue
  matrix.color565(102, 51, 253),    // 5 Purple
  0,                                // 6 Black
  65535,                            // 7 White
  matrix.color565(72, 72, 72),      // 8 Gray
  matrix.color565(28, 33, 168),     // 9 Duke Blue
  matrix.color565(0, 255, 255),     // 10 Cyan
  matrix.color565(255, 0, 255),     // 11 Magenta
};

// Active background palette (up to 6 colours) and per-digit ink colours
uint16_t palette[6];
uint16_t ink_color[4] = {WHITE, WHITE, WHITE, WHITE};
int palette_size = 6;
int current_palette = 0; // which named palette is active (0 = default)
int current_pattern = 0; // which background pattern is active

// Scroll counter used to animate moving patterns
int scroll = 0;

// Forward declaration required because update_digits_task references access_rtc
// before the function definition appears below
void access_rtc();

// SCHEDULER SETUP
// update_digits_task : fires every 1000 ms to refresh digits[] from the RTC.
// draw_current_pattern is a function pointer (set by switch_pattern) called
// directly in loop() every frame to ensure the back buffer is always fresh.
Scheduler face_scheduler;
Task update_digits_task(1000, -1, &access_rtc);

typedef void (*PatternFn)();
PatternFn draw_current_pattern = nullptr;

// Include pattern and palette helper functions (must come after declarations above)
#include "face_task_list.h"

// ---- RTC HELPERS -----------------------------------------------------------

// Read the RTC once per second and update digits[] and date_array[].
//
// Double-read validation: two consecutive reads must agree on hour and minute
// before updating display state.  The DS3231 BCD registers are not read
// atomically; a counter increment mid-read, or I2C timing disturbed by the
// matrix DMA interrupt, can produce a snapshot where hour and minute belong to
// different seconds.  Comparing two reads taken microseconds apart (far less
// than one RTC tick) discards any such incoherent pair.
void access_rtc() {
  DateTime a = rtc.now();
  DateTime b = rtc.now();
  if (a.hour() != b.hour() || a.minute() != b.minute()) return;

  now = a;
  int m  = now.minute();
  int hr = now.twelveHour();  // 1–12, never 0

  if (m > 59) return;  // BCD rollover guard: minute=60 at the top of each hour

  digits[0] = (hr < 10) ? 0 : 1;
  digits[1] = (hr < 10) ? hr : hr - 10;
  digits[2] = m / 10;
  digits[3] = m % 10;

  for (int i = 0; i < 3; i++)
    date_array[i] = months[now.month() - 1][i];
  date_array[3] = now.day() / 10;
  date_array[4] = now.day() % 10;
}

// Return the DateTime of the second Sunday in March (DST start in North America)
DateTime calc_dst_start() {
  now = rtc.now();
  DateTime this_day;
  int day_of_month = 1;
  for (int num_sunday = 0; num_sunday < 2; ) {
    this_day = DateTime(now.year(), 3, day_of_month);
    if (!this_day.dayOfTheWeek()) { // Sunday = 0
      num_sunday++;
    } else {
      day_of_month++;
    }
  }
  return this_day;
}

// Return the DateTime of the first Sunday in November (DST end in North America)
DateTime calc_dst_end() {
  now = rtc.now();
  int day_of_month = 1;
  DateTime this_day;
  for (int num_sunday = 0; num_sunday < 1; ) {
    this_day = DateTime(now.year(), 11, day_of_month);
    if (!this_day.dayOfTheWeek()) {
      num_sunday++;
    } else {
      day_of_month++;
    }
  }
  return this_day;
}

// Return true if the current time falls within North American DST
bool check_dst() {
  return (calc_dst_start() <= rtc.now()) && (rtc.now() <= calc_dst_end());
}

// ---- COORDINATE HELPERS ----------------------------------------------------

// Map a flat pixel index (0-79) to its X coordinate within an 8-wide glyph
int trans_x(int pos)      { return pos % 8; }
// Map a flat pixel index (0-79) to its Y coordinate within a 10-tall glyph
int trans_y(int pos)      { return pos / 8; }
// Map a flat pixel index (0-14) to its X coordinate within a 3-wide small glyph
int trans_x_smol(int pos) { return pos % 3; }
// Map a flat pixel index (0-14) to its Y coordinate within a 5-tall small glyph
int trans_y_smol(int pos) { return pos / 3; }

// ---- DISPLAY FUNCTIONS -----------------------------------------------------

// Draw the four time-digit glyphs (foreground / lit pixels only) in ink_color[].
//
// fillScreen(0) at the start of every frame provides a clean slate, so unlit
// digit pixels default to black or show the background pattern beneath them.
// The old bg=true "reverse draw" path is not needed in this pipeline.
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
    for (int dig = 0; dig < 4; dig++) {
      for (int i = 0; i < 80; i++) {
        if (num[digits[dig]][i])
          matrix.drawPixel(trans_x(i) + dig * 8, trans_y(i), ink_color[dig]);
      }
    }
    return;
  }

  // Single-digit hour (1–9) with colon.
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

// Draw the date (3-char month abbreviation + 2-digit day) in the bottom rows
// using small 3×5 glyphs at the given color
void display_date(uint16_t color) {
  // Month abbreviation: three 3×5 letter glyphs starting at x=10, y=11
  for (int place = 0; place < 3; place++) {
    for (int i = 0; i < 15; i++) {
      if (letters[date_array[place]][i]) {
        matrix.drawPixel(10 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
  // Day number: two 3×5 digit glyphs after the month abbreviation
  for (int place = 3; place < 5; place++) {
    for (int i = 0; i < 15; i++) {
      if (small_num[date_array[place]][i]) {
        matrix.drawPixel(11 + trans_x_smol(i) + place * 4, trans_y_smol(i) + 11, color);
      }
    }
  }
}

// ---- SETUP -----------------------------------------------------------------

void setup(void) {
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
    // DO NOT CONTINUE if matrix setup encountered an error.
    for (;;);
  }

  // Perform an initial RTC read so digits[] and date_array[] are populated
  access_rtc();

  // Select starting colour palette and background pattern
  current_palette = 1; // rainbow palette
  change_palette();
  switch_pattern(0);   // scrolling diagonal stripes

  // Register and enable the scheduler tasks
  face_scheduler.addTask(update_digits_task);
  update_digits_task.enable();

  matrix.show();
}

// ---- LOOP ------------------------------------------------------------------

void loop() {
  // Run the scheduler every iteration so the RTC read (1000 ms) is serviced
  // at its own cadence, independent of the render frame rate.
  face_scheduler.execute();

  // Enforce the target frame rate.
  static unsigned long last_frame = 0;
  unsigned long now_ms = millis();
  if ((now_ms - last_frame) < FRAME_MS) return;
  last_frame = now_ms;

  // Advance the animation counter every SCROLL_DIVIDER frames so the
  // background drifts at a pleasant pace rather than strobing each frame.
  static uint8_t scroll_div = 0;
  if (++scroll_div >= SCROLL_DIVIDER) {
    scroll_div = 0;
    scroll = (scroll + 1) % 600;  // 600 = LCM(4,6)×50 — no colour jump at wrap
  }

  // ── Frame rendering ──────────────────────────────────────────────────────
  // Protomatter double-buffering SWAPS the two frame buffers on show().
  // After the swap the new back buffer contains the former front buffer's
  // pixels (two frames ago), not a clean slate.  Without this explicit clear,
  // unpainted pixels retain their stale colour and the display oscillates at
  // 10 Hz — exactly the "jumping" symptom.
  matrix.fillScreen(0);                              // 1. clean slate

  if (draw_current_pattern) draw_current_pattern();  // 2. background pattern
  display_time(true);                                // 3. time digits
  matrix.drawFastHLine(0, 10, 32, 0);               // 4. separator row
  display_date(matrix.color565(128, 128, 128));      // 5. date

  matrix.show();                                     // 6. swap to display
}
