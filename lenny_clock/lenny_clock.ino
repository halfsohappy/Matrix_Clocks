// lenny_clock.ino
// An RGB LED matrix clock driven by an Adafruit Protomatter matrix and a DS3231 RTC.
// Displays the current time (HH:MM) in the top 10 rows and the date (Mon DD) in
// the bottom rows of a 32×16 pixel matrix.  Supports multiple background colour
// patterns with digits "cut out" of the background.

#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "font_array.h"

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

// Real-time clock object and current timestamp
RTC_DS3231 rtc;
DateTime now;

// Adafruit Protomatter matrix: 32 px wide, 4-bit colour depth, single chain,
// 3 address pins (height inferred as 16), no double-buffering
Adafruit_Protomatter matrix(
  32,          // Width of matrix (or matrix chain) in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  3, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  false);      // No double-buffering here (see "doublebuffer" example)

// Rainbow (pride flag) colours, Duke University palette, and a two-tone "yelp" palette
uint16_t pride[] = {matrix.color565(253,0,0), matrix.color565(253, 152, 0), matrix.color565(254, 254, 0),
matrix.color565(51, 254, 0), matrix.color565(0, 151, 253), matrix.color565(102, 51, 253), 0};

uint16_t duke[] = {matrix.color565(28, 33, 168), 65535, matrix.color565(72, 72, 72)};
uint16_t yelp[] = {pride[5],0};

// Scale all palette colours by a brightness factor (0.0 = off, 1.0 = full)
void update_brightness(float br){
  uint16_t pride2[] = {matrix.color565(253*br,0,0), matrix.color565(253*br, 152*br, 0), matrix.color565(254*br, 254*br, 0),
    matrix.color565(51*br, 254*br, 0), matrix.color565(0, 151*br, 253*br), matrix.color565(102*br, 51*br, 253*br), 0};
  for(int i =0; i<6;i++){pride[i] = pride2[i];}
  duke[0] = matrix.color565(28*br, 33*br, 168*br);
  duke[1] = matrix.color565(255*br, 255*br, 255*br);
  duke[2] = matrix.color565(72*br, 72*br, 72*br);
  yelp[0] = pride[5];
}

int scroll = 0; // counter used to animate scrolling patterns

// Draw diagonal rainbow stripes that shift by position_color each call
void scroll_color(int position_color){
  for(int i=0; i<48; i++){
  matrix.drawLine(0,i,i,0,pride[(abs(i - position_color) % 6)]);
  }
  scroll++;
  if(scroll == 100){scroll = 0;}
}

// Draw diagonal Duke blue/white/grey stripes
void scroll_blue_white(int position_color){
  for(int i=0; i<48; i++){
    matrix.drawLine(0,i,i,0,duke[(abs(i - position_color) % 3)]);
  }
  if(scroll == 100){scroll = 0;}
}

// Fill the display with random purple or black pixels
void rand_yelp(int position_color){
  for(int x=0; x<32; x++){
    for(int y=0; y<16; y++){
    matrix.drawPixel(x,y,yelp[int(random(0,2))]);
  }
  if(scroll == 100){scroll = 0;}
}}


// Helper: map a flat pixel index (0-79) to its X coordinate within an 8-wide glyph
int trans_x(int pos){
  return pos % 8;
}
// Helper: map a flat pixel index (0-79) to its Y coordinate within a 10-tall glyph
int trans_y(int pos){
  return pos/8;
}
// Helper: map a flat pixel index (0-14) to its X coordinate within a 3-wide small glyph
int trans_x_smol(int pos){
  return pos % 3;
}
// Helper: map a flat pixel index (0-14) to its Y coordinate within a 5-tall small glyph
int trans_y_smol(int pos){
  return pos/3;
}

// digits[0..3] holds the four display digits: [H1, H2, M1, M2]
int digits[] = {1,0,2,3};

// Read the RTC and update the digits[] array with the current 12-hour time
void update_digits(){
  now = rtc.now();
  if(now.twelveHour() < 10){digits[1] = now.twelveHour(); digits[0] = 0;}
  else{digits[0] = 1; digits[1] = now.twelveHour()-10;}
  digits[2] = (now.minute() / 10);
  digits[3] = now.minute() % 10;
}

// Paint the background (non-digit) pixels of one digit position with color
void s_cover_background(int position, uint16_t color){
    for(int i = 0; i<80; i++){
      if(!num[digits[position]][i]){
        matrix.drawPixel((trans_x(i) + position*8), trans_y(i), color);
      }
    }
  }

// Paint the background pixels of all four digit positions with color
void cover_background(uint16_t color){
  for(int dig = 0; dig < 4; dig++){
    s_cover_background(dig, color);
   }
}

// Paint the digit (foreground) pixels of one digit position with color
void s_cover_digits(int position, uint16_t color){
    for(int i = 0; i<80; i++){
      if(num[digits[position]][i]){
        matrix.drawPixel((trans_x(i) + position*8), trans_y(i), color);
      }
    }
  }

// Paint the digit pixels of all four digit positions with color
void cover_digits(uint16_t color){
  for(int dig = 0; dig < 4; dig++){
    s_cover_digits(dig, color);
  }
}

// Fill an 8×11 block for one digit column with a solid colour
void color_block(int pos, uint16_t color){
  matrix.fillRect(pos*8, 0, 8, 11, color);
}

// Background pattern: red/blue/yellow/white blocks with black digit cutouts
void four_RBYW(){
  color_block(0, pride[0]);
  color_block(1, pride[4]);
  color_block(2, pride[2]);
  color_block(3, matrix.color565(255,255,255));
}

// Background pattern: non-binary flag colours (yellow/white/purple/black) with
// digit shapes "punched out" in contrasting colours
void enby(){
  color_block(0, pride[2]);
  color_block(1, 65535);
  color_block(2, pride[5]);
  color_block(3, 0);
  s_cover_digits(0, 0);
  s_cover_digits(1, 0);
  s_cover_digits(2, 0);
  s_cover_digits(3, 65535);
}

// Background pattern: each digit column has a contrasting foreground colour
void contrast_color(){
  color_block(0, pride[0]); s_cover_digits(0, pride[3]);
  color_block(1, pride[4]); s_cover_digits(1, pride[1]);
  color_block(2, pride[2]); s_cover_digits(2, pride[5]);
  color_block(3, matrix.color565(255,255,255)); s_cover_digits(3,0);
}
// date_places[0..2] = letter indices for the 3-char month abbreviation
// date_places[3]    = tens digit of day
// date_places[4]    = ones digit of day
int date_places[] = {0,0,0,0,0};

// Draw the month abbreviation and day number in the bottom 5 rows of the matrix
void date(uint16_t color){
  for(int letter = 0; letter <3; letter++){
    date_places[letter] = months[now.month()-1][letter];
  }
  date_places[3] = (now.day() / 10);
  date_places[4] = now.day() % 10;
  // Draw the three month-abbreviation letters (3×5 pixel glyphs) starting at x=10
  for(int place = 0; place<3; place++){
    for(int i = 0; i<15; i++){
      if(letters[date_places[place]][i]){
        matrix.drawPixel(10+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, color);
      }
    }
  }
  // Draw the two day-number digits (3×5 pixel glyphs) after the month letters
  for(int place = 3; place<5; place++){
    for(int i = 0; i<15; i++){
      if(small_num[date_places[place]][i]){
        matrix.drawPixel(11+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, color);
      }
    }
  }
}

// Indices spelling "BLAZE IT" for a special text overlay
int blaze_num[] = {1,11,0,25,4,8,19};

// Draw "BLAZE IT" text in the lower rows using pride[0] (red)
void blaze_it(){
  for(int place = 0; place<5; place++){
    for(int i = 0; i<15; i++){
      if(letters[blaze_num[place]][i]){
        matrix.drawPixel(1+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, pride[0]);
      }
    }
  }
  for(int place = 5; place<7; place++){
    for(int i = 0; i<15; i++){
      if(letters[blaze_num[place]][i]){
        matrix.drawPixel(2+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, pride[0]);
      }
    }
  }
}

// Indices spelling "BIRTHDAY" for a birthday text overlay
int birthday_num[] = {1,8,17,19,7,3,0,24};

// Erase "BIRTHDAY" text from the lower rows (draws black over letter pixels)
void birthday(){
  for(int place = 0; place<8; place++){
    for(int i = 0; i<15; i++){
      if(!letters[birthday_num[place]][i]){
        matrix.drawPixel(1+trans_x_smol(i)+ place*4, trans_y_smol(i)+11, 0);
      }
    }
    matrix.drawFastVLine(place*4, 11, 5, 0);
  }
}

void setup(void) {
  Serial.begin(9600);
  rtc.begin();

  delay(100);
  now = rtc.now();

  // Initialize the Protomatter matrix
  ProtomatterStatus status = matrix.begin();
  matrix.setRotation(rot);

  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error.
    for(;;);
  }
}


// LOOP - RUNS REPEATEDLY AFTER SETUP --------------------------------------
void loop() {
  // Refresh time digits from RTC
  update_digits();

  // Draw enby-flag background pattern with digit cutouts
  enby();

  // Draw a black separator line between the time and date areas
  matrix.drawFastHLine(0, 10, 32, 0);

  // Draw the date (month abbreviation + day number) in grey
  date(matrix.color565(128, 128, 128));

  // Push the frame buffer to the physical display
  matrix.show();
  delay(100);
}
