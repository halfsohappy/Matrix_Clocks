# Matrix Clocks

RGB LED matrix clocks driven by an [Adafruit Protomatter](https://github.com/adafruit/Adafruit_Protomatter) matrix and a DS3231 real-time clock (RTC) module.  The display is a 32 × 16 pixel RGB LED panel.  The top 10 rows show the current time in a custom 8 × 10 pixel font, a thin separator line divides the panel, and the bottom 5 rows show the date as a 3-letter month abbreviation and 2-digit day using a 3 × 5 pixel font.

```
┌────────────────────────────────┐
│  H H  H H  :  M M  M M        │  rows 0-9   (time, 8×10 glyphs)
├────────────────────────────────┤  row 10     (separator)
│        M M M  D D             │  rows 11-15 (date, 3×5 glyphs)
└────────────────────────────────┘
```

---

## Repository structure

```
Matrix_Clocks/
├── matrix_clock/              ← ★ START HERE — the merged, fully-featured sketch
│   ├── matrix_clock.ino  – Merged clock: all palettes, patterns, buttons, DST, brightness
│   ├── face_task_list.h  – Pattern callbacks and palette/ink management
│   ├── font_array.h      – 8×10 digit glyphs + colon glyph + 3×5 small fonts
│   └── my_char.h         – Arduino WCharacter.h compatibility header
│
├── lenny_clock/
│   ├── lenny_clock.ino   – Simple clock with solid colour-block background patterns
│   ├── font_array.h      – 8×10 digit glyphs (flat int[11][80] arrays) + 3×5 small fonts
│   └── my_char.h         – Arduino WCharacter.h compatibility header
│
└── ella_clock/
    ├── ella_clock.ino    – Feature-rich clock with TaskScheduler, DST detection,
    │                       11 named colour palettes, and 8 background patterns
    ├── face_task_list.h  – Pattern callbacks and palette/ink management
    ├── font_array.h      – Same glyphs as lenny_clock, plus letter array
    ├── my_char.h         – Arduino WCharacter.h compatibility header
    │
    └── ella_new/
        └── ella_new.ino  – Rewritten architecture using a two-layer pixel buffer
                            (base_pixels for background, top_pixels for foreground)
```

---

## Hardware

| Component | Notes |
|-----------|-------|
| 32 × 16 RGB LED HUB75 matrix | Any compatible panel; tested at 32 wide with 5 address pins |
| Adafruit Metro M4 Express *or* Adafruit Feather RP2040 | Other Adafruit M4/M0/RP2040 boards should also work |
| DS3231 RTC module | Communicates over I²C |
| USB-C or LiPo power | The LED matrix can draw significant current — size your supply accordingly |

---

## Libraries

Install these via the Arduino IDE Library Manager (or `arduino-cli lib install`):

| Library | Purpose |
|---------|---------|
| **Adafruit Protomatter** | Drives the HUB75 RGB LED matrix |
| **Adafruit GFX Library** | Drawing primitives (lines, rectangles, pixels) |
| **RTClib** | DS3231 real-time clock |
| **TaskScheduler** | Non-blocking cooperative multitasking (ella_clock only) |

---

## Pin configuration

Both sketches support two hardware targets.  Only one pin block should be active at a time — comment out the unused one.

### Metro M4 (default in `lenny_clock` and `ella_clock`)

```cpp
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;
int rot = 2;
```

### Feather RP2040 (default in `ella_new`)

```cpp
uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
uint8_t addrPins[] = {25, 24, 29, 28};
uint8_t clockPin   = 13;
uint8_t latchPin   = 1;
uint8_t oePin      = 0;
int rot = 0;
```

> **Note:** `addrPins` for the Metro M4 has **5** entries (supports up to 32 rows), while the RP2040 version has **4** entries (supports up to 16 rows).  The Protomatter library infers the panel height from the number of address pins.

---

## Setting the time

The DS3231 RTC module keeps time with a coin-cell battery.  If this is the first time using the module, or after a battery swap, set the time by uploading a small sketch that calls:

```cpp
rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
```

This sets the RTC to the build time of the sketch.  Once the time is set it is stored in the RTC and persists across power cycles.

---

## `matrix_clock` — the merged sketch

`matrix_clock` is the recommended sketch.  It combines every feature from `lenny_clock` and `ella_clock` into one file, with a clearly labelled configuration block at the top.

### Compile-time configuration

Open `matrix_clock/matrix_clock.ino` and edit the `#define` block before uploading:

```cpp
// Starting palette (1–11).  Cycle at runtime with Button A.
#define DEFAULT_PALETTE  1

// Starting pattern (0–7).  Cycle at runtime with Button B.
#define DEFAULT_PATTERN  0

// Draw a colon between the hour and minute digits.  1 = on, 0 = off.
#define ENABLE_COLON     1

// Detect North-American DST and offset the displayed hour.  1 = on, 0 = off.
#define ENABLE_DST       0

// Overall brightness, 0.0 (off) to 1.0 (full).
#define BRIGHTNESS       1.0

// Button pins — wire one leg to the pin and the other to GND.
#define BTN_PALETTE_PIN  A0
#define BTN_PATTERN_PIN  A1
```

### Runtime button control

| Button | Connected to | Action |
|--------|-------------|--------|
| Button A | `BTN_PALETTE_PIN` (default `A0`) | Cycles to the next colour palette (1 → 2 → … → 11 → 1) |
| Button B | `BTN_PATTERN_PIN` (default `A1`) | Cycles to the next background pattern (0 → 1 → … → 7 → 0) |

Both buttons are **active-LOW with internal pull-up** resistors enabled — wire one side to the pin and the other side to `GND`.  No external resistors are needed.  Presses are debounced with a 200 ms window (configurable via `BTN_DEBOUNCE_MS`).

### Palettes (Button A)

| # | Name | Background | Digits |
|---|------|-----------|--------|
| 1 | Rainbow | Red/Orange/Yellow/Green/Blue/Purple | Black |
| 2 | Rainbow | Red/Orange/Yellow/Green/Blue/Purple | White |
| 3 | RBYW | Red/Blue/Yellow/White | Black |
| 4 | Pure RGB | Pure Red/Green/Blue/White | Black |
| 5 | CMY | Cyan/Magenta/Yellow/Black | White |
| 6 | Pastel | Six soft pink/green/purple tones | Black |
| 7 | Wilderness | Six earthy brown/green tones | Black |
| 8 | Duke | Duke Blue/Grey/White/Duke Blue | Black |
| 9 | Purple/Yellow | Purple and Yellow alternating | Yellow/Purple per digit |
| 10 | Orange/Cyan | Orange and Blue alternating | Blue/Orange per digit |
| 11 | Monochrome | Black/White alternating | Grey |

### Patterns (Button B)

| # | Name | Description |
|---|------|-------------|
| 0 | Scroll diagonal | Animated rainbow diagonal stripes |
| 1 | Static diagonal | Non-animated diagonal stripes |
| 2 | Colour blocks | Solid bands, one per palette colour |
| 3 | H-thin | Thin horizontal stripes, one row per colour |
| 4 | H-thick | Thick horizontal bands |
| 5 | V-thin | Thin vertical stripes |
| 6 | V-thick | Thick vertical bands |
| 7 | Random | Random per-pixel palette colour each frame |

### Special date-row overlays

The default date row can be replaced with a special message by swapping the `display_date()` call in `loop()`:

```cpp
// In loop(), replace:
display_date(matrix.color565(128, 128, 128));

// With one of:
blaze_it();   // "BLAZE IT" in red  (e.g. for April 20)
birthday();   // "BIRTHDAY" erased over the background
```

### Brightness

```cpp
// Dim to 50% — edit the define at the top of the sketch:
#define BRIGHTNESS  0.5

// Or call at runtime after setup() to re-dim dynamically:
update_brightness(0.5);
change_palette();  // reload so palette[] uses the new scaled colours
```

---

## Sketch descriptions

### `lenny_clock`

The simpler of the two main sketches.  Each iteration of `loop()`:

1. Reads the current time from the RTC.
2. Paints the background using the active pattern (default: `enby()` — non-binary flag colours).
3. "Punches out" the digit shapes by drawing over them in a contrasting colour.
4. Draws a separator line.
5. Draws the date.
6. Calls `matrix.show()`.

Available pattern helpers: `enby()`, `four_RBYW()`, `contrast_color()`, `scroll_color()`, `scroll_blue_white()`, `rand_yelp()`.

### `ella_clock`

A more advanced clock that uses the [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) library for non-blocking operation:

| Task | Interval | Purpose |
|------|----------|---------|
| `update_digits_task` | 50 ms | Reads the RTC and refreshes `digits[]` and `date_array[]` |
| `face_task` | 25–100 ms | Runs the active background pattern callback |

`loop()` calls `face_scheduler.execute()` then overlays the time and date on top of whatever the pattern has drawn.

**Palettes (11):** rainbow (black/white digits), RBYW, RGBW, CMYK-like, pastel pink/green, wilderness earth tones, Duke University, purple/yellow, orange/cyan, monochrome.

**Patterns (8):** scrolling diagonal, static diagonal, colour blocks, thin/thick horizontal stripes, thin/thick vertical stripes, random.

**DST support:** `check_dst()` detects North American DST (second Sunday in March → first Sunday in November).

### `ella_new`

A rewritten architecture that separates background and foreground rendering via two pixel-buffer arrays:

- **`base_pixels[16][32]`** — background colour for every pixel, filled by pattern functions such as `diagonal_rainbow()`, `RBYW()`, `duke_checker()`.
- **`top_pixels[16][32]`** — foreground colour used when drawing digit glyphs, allowing per-pixel tinting of the text.

`loop()` fills `base_pixels`, calls `refill_base()` to flush it to the matrix, then draws digits and the date using the `draw_digit()` / `draw_small()` / `draw_date()` helpers.  The rainbow background is animated via a circular linked-list of 6 colour nodes (`rainbow_link_setup()`).

---

## Differences between `lenny_clock` and `ella_clock`

Both sketches produce the same visual output (time + date on a 32 × 16 matrix) but differ significantly in complexity and capability.

### At a glance

| Feature | `lenny_clock` | `ella_clock` |
|---------|--------------|-------------|
| Extra library | — | TaskScheduler |
| Colour palettes | 3 hard-coded arrays (`pride`, `duke`, `yelp`) | 11 named, runtime-switchable palettes |
| Background patterns | 6 functions called directly in `loop()` | 8 pattern callbacks managed by TaskScheduler |
| Per-digit ink colour | Fixed (set per pattern function) | 4-element `ink_color[]` array, palette-driven |
| RTC read | Every loop iteration (`update_digits()`) | Scheduled Task every 50 ms (`access_rtc()`) |
| Date fields | Computed inside `date()` on each call | Cached in `date_array[]` by the scheduled task |
| Loop style | Blocking — sequential + `delay(100)` | Non-blocking — `face_scheduler.execute()` |
| Colon separator | None — digits fill fixed 8 px columns | Optional: shifts single-digit hour and draws colon dots |
| DST detection | None | `check_dst()` for North American DST |
| Brightness scaling | `update_brightness(float)` scales `pride[]`/`duke[]` | None |
| Special text overlays | `blaze_it()`, `birthday()` in date row | None |
| Font table size | `int num[11][80]` — digits 0–9 plus a colon glyph | `int num[10][80]` — digits 0–9 only |
| Default display | Non-binary flag colours (`enby()`) | Rainbow palette + scrolling diagonal stripes |

### Colour system

**`lenny_clock`** hard-codes three colour arrays at compile time:

- `pride[7]` — the six rainbow colours (red, orange, yellow, green, blue, purple) plus black as a seventh entry  
- `duke[3]` — Duke Blue, white, grey  
- `yelp[2]` — purple and black  

Pattern functions (`enby()`, `four_RBYW()`, etc.) pick directly from these arrays. `update_brightness(float)` can scale them all at once, but switching colour schemes requires changing code.

**`ella_clock`** uses a two-layer system:

- `colors[12]` — a full named palette (red, orange, yellow, green, blue, purple, black, white, grey, Duke Blue, cyan, magenta)  
- `palette[6]` — the *active* background colours, loaded from any of 11 pre-defined palettes by `change_palette()`  
- `ink_color[4]` — per-digit foreground colours, also set by the active palette  

Calling `change_pal_helper()` cycles through all 11 palettes at runtime without re-flashing the board.

### Rendering model

**`lenny_clock`** uses a **punch-out** approach: each pattern function fills a column with a solid background colour, then calls `s_cover_digits()` to overdraw the digit pixels in a contrasting colour. Background and foreground are painted in the same function call.

**`ella_clock`** uses an **overlay** approach: `face_task` runs the background pattern (fills the whole clock area with the active palette design), then `display_time()` draws only the digit-pixel positions in `ink_color[]` on top. Background and foreground are independent.

### Task scheduling

**`lenny_clock`** runs everything sequentially inside `loop()`:

```
loop(): update_digits() → enby() → separator → date() → show() → delay(100)
```

Every step blocks until the previous one finishes. The 100 ms `delay()` sets the update rate.

**`ella_clock`** uses the TaskScheduler library so each concern runs on its own timer:

```
face_scheduler runs:
  update_digits_task  every  50 ms  → access_rtc()
  face_task           every 25-100 ms → active pattern callback

loop(): execute() → display_time() → separator → display_date() → show() → scroll++
```

The pattern interval is set by `switch_pattern()` and varies by pattern (e.g. 100 ms for animated scrolling, 25 ms for static). There is no `delay()` in `loop()`, so the board stays responsive.

### When to use which sketch

- Use **`lenny_clock`** if you want a simple, easy-to-modify sketch with no extra libraries and a fixed colour scheme. It is the right starting point for customising the look.
- Use **`ella_clock`** if you want runtime palette/pattern switching, a proper colon separator, or DST-aware time display. The TaskScheduler architecture also makes it easier to add button inputs or other non-blocking behaviour later.
- Use **`ella_new`** (described in the Sketch descriptions section above) if you want per-pixel control over the foreground colour — for example, digits that show a different hue per pixel, or text that visually blends into the animated rainbow background.

---

## Customising the display

### Changing the background pattern (lenny_clock)

In `loop()`, replace the `enby()` call with any of the other pattern functions:

```cpp
// Examples:
four_RBYW();           // four colour blocks
contrast_color();      // per-digit contrasting colours
scroll_color(scroll);  // animated rainbow diagonal
```

### Changing the palette / pattern (ella_clock)

Change the starting palette and pattern in `setup()`:

```cpp
current_palette = 3; // Red/Blue/Yellow/White
change_palette();
switch_pattern(2);   // solid colour blocks
```

Valid `current_palette` values: 1–11.  Valid `switch_pattern` values: 0–7.

### Changing the foreground colour (ella_new)

Assign a different colour to `top_pixels` before calling `refill_base()`:

```cpp
// White digits:
for (int y = 0; y < 16; y++)
  for (int x = 0; x < 32; x++)
    top_pixels[y][x] = 65535;

// Inverted background (digits show the background colour):
for (int y = 0; y < 16; y++)
  for (int x = 0; x < 32; x++)
    top_pixels[y][x] = base_pixels[y][x]; // digits blend with background
```

---

## Troubleshooting

| Symptom | Likely cause |
|---------|-------------|
| Nothing displayed | Check matrix wiring; the sketch halts if `matrix.begin()` returns an error |
| Wrong time shown | RTC not set; run the `adjust()` snippet above |
| Garbled digits | Wrong pin mapping; verify the active pin block matches your board |
| Dim display | Power supply too weak; HUB75 panels can draw 2–4 A at full brightness |
