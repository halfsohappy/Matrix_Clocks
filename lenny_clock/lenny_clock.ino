#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "font_array.h"
  // MATRIX PINS:
  uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
  uint8_t addrPins[] = {17, 18, 19, 20, 21};
  uint8_t clockPin   = 14;
  uint8_t latchPin   = 15;
  uint8_t oePin      = 16;
  int rot = 2;
  
  //RP2040FEATHER PINS:
  // uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
  // uint8_t addrPins[] = {25, 24, 29, 28};
  // uint8_t clockPin   = 13;
  // uint8_t latchPin   = 1;
  // uint8_t oePin      = 0;
  // int rot = 0;
RTC_DS3231 rtc;
DateTime now;
Adafruit_Protomatter matrix(
  32,          // Width of matrix (or matrix chain) in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  3, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  false);      // No double-buffering here (see "doublebuffer" example)

uint16_t pride[] = {matrix.color565(253,0,0), matrix.color565(253, 152, 0), matrix.color565(254, 254, 0),
matrix.color565(51, 254, 0), matrix.color565(0, 151, 253), matrix.color565(102, 51, 253), 0};

uint16_t duke[] = {matrix.color565(28, 33, 168), 65535, matrix.color565(72, 72, 72)};
uint16_t yelp[] = {pride[5],0};
void update_brightness(float br){
  uint16_t pride2[] = {matrix.color565(253*br,0,0), matrix.color565(253*br, 152*br, 0), matrix.color565(254*br, 254*br, 0),
    matrix.color565(51*br, 254*br, 0), matrix.color565(0, 151*br, 253*br), matrix.color565(102*br, 51*br, 253*br), 0};
  for(int i =0; i<6;i++){pride[i] = pride2[i];}
  duke[0] = matrix.color565(28*br, 33*br, 168*br);
  duke[1] = matrix.color565(255*br, 255*br, 255*br);
  duke[2] = matrix.color565(72*br, 72*br, 72*br);
  yelp[0] = pride[5];
}

int scroll = 0;

void scroll_color(int position_color){
  for(int i=0; i<48; i++){
  matrix.drawLine(0,i,i,0,pride[(abs(i - position_color) % 6)]);
  }
  scroll++;
  if(scroll == 100){scroll = 0;}
}

void scroll_blue_white(int position_color){
  for(int i=0; i<48; i++){
    matrix.drawLine(0,i,i,0,duke[(abs(i - position_color) % 3)]);
  }
  //scroll++;
  if(scroll == 100){scroll = 0;}
}
void rand_yelp(int position_color){
  for(int x=0; x<32; x++){
    for(int y=0; y<16; y++){
    matrix.drawPixel(x,y,yelp[int(random(0,2))]);
  }
  //scroll++;
  if(scroll == 100){scroll = 0;}
}}


int trans_x(int pos){
  return pos % 8;
}
int trans_y(int pos){
  return pos/8;
}
int trans_x_smol(int pos){
  return pos % 3;
}
int trans_y_smol(int pos){
  return pos/3;
}

int digits[] = {1,0,2,3};

void update_digits(){//fills 
  now = rtc.now();
  if(now.twelveHour() < 10){digits[1] = now.twelveHour(); digits[0] = 0;}
  else{digits[0] = 1; digits[1] = now.twelveHour()-10;}
  digits[2] = (now.minute() / 10);
  digits[3] = now.minute() % 10; 
}

void s_cover_background(int position, uint16_t color){
  // if(digits[0] == 0){
  //   if(position ==0){matrix.fillRect(0, 0, 4, 10, color);}

  //   if(position == 1){
  //     for(int i = 0; i<80; i++){
  //       if(!num[digits[position]][i]){
  //         matrix.drawPixel((trans_x(i) + position*8)-4, trans_y(i), color);
  //       }
  //     }
  //     matrix.fillRect(12, 0, 4, 2, color);
  //     matrix.fillRect(12, 8, 4, 2, color);
  //     matrix.fillRect(12, 0, 1, 10, color);
  //     matrix.fillRect(12, 4, 4, 2, color);
  //     matrix.fillRect(15, 0, 1, 10, color);
  //   }
  //   if(position == 2 || position == 3){
  //     for(int i = 0; i<80; i++){
  //       if(!num[digits[position]][i]){
  //         matrix.drawPixel((trans_x(i) + position*8), trans_y(i),color);
  //       }
  //     }
  //   }
  // }
  //else{
    for(int i = 0; i<80; i++){
      if(!num[digits[position]][i]){
        matrix.drawPixel((trans_x(i) + position*8), trans_y(i), color);
      }
    }
  }
//}

void cover_background(uint16_t color){
  for(int dig = 0; dig < 4; dig++){
    s_cover_background(dig, color);
   }
}

void s_cover_digits(int position, uint16_t color){
  // if(digits[0] == 0){
  //   if(position ==0){matrix.fillRect(0, 0, 4, 10, color);}

  //   if(position == 1){
  //     for(int i = 0; i<80; i++){
  //       if(!num[digits[position]][i]){
  //         matrix.drawPixel((trans_x(i) + position*8)-4, trans_y(i), color);
  //       }
  //     }
  //     matrix.fillRect(12, 0, 4, 2, color);
  //     matrix.fillRect(12, 8, 4, 2, color);
  //     matrix.fillRect(12, 0, 1, 10, color);
  //     matrix.fillRect(12, 4, 4, 2, color);
  //     matrix.fillRect(15, 0, 1, 10, color);
  //   }
  //   if(position == 2 || position == 3){
  //     for(int i = 0; i<80; i++){
  //       if(num[digits[position]][i]){
  //         matrix.drawPixel((trans_x(i) + position*8), trans_y(i),color);
  //       }
  //     }
  //   }
  // }
  // else{
    for(int i = 0; i<80; i++){
      if(num[digits[position]][i]){
        matrix.drawPixel((trans_x(i) + position*8), trans_y(i), color);
      }
    }
  }
//}

void cover_digits(uint16_t color){
  for(int dig = 0; dig < 4; dig++){
    s_cover_digits(dig, color);
  }
}

void color_block(int pos, uint16_t color){
  matrix.fillRect(pos*8, 0, 8, 11, color);
}

void four_RBYW(){
  color_block(0, pride[0]);
  color_block(1, pride[4]);
  color_block(2, pride[2]);
  color_block(3, matrix.color565(255,255,255));
}

void enby(){
  color_block(0, pride[2]);
  color_block(1, 65535);
  color_block(2, pride[5]);
  color_block(3, 0);
  //s_cover_digits(0, pride[5]);
  s_cover_digits(0, 0);
  s_cover_digits(1, 0);
  //s_cover_digits(2, pride[2]);
  s_cover_digits(2, 0);
  s_cover_digits(3, 65535);
}

void contrast_color(){
  color_block(0, pride[0]); s_cover_digits(0, pride[3]);
  color_block(1, pride[4]); s_cover_digits(1, pride[1]);
  color_block(2, pride[2]); s_cover_digits(2, pride[5]);
  color_block(3, matrix.color565(255,255,255)); s_cover_digits(3,0);
}
int date_places[] = {0,0,0,0,0};

void date(uint16_t color){
  for(int letter = 0; letter <3; letter++){
    date_places[letter] = months[now.month()-1][letter];
  }
  date_places[3] = (now.day() / 10);
  date_places[4] = now.day() % 10;
  for(int place = 0; place<3; place++){
    for(int i = 0; i<15; i++){
      if(letters[date_places[place]][i]){
        matrix.drawPixel(10+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, color);
      }
    }
  }
  for(int place = 3; place<5; place++){
    for(int i = 0; i<15; i++){
      if(small_num[date_places[place]][i]){
        matrix.drawPixel(11+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, color);
      }
    }
  }
}

int blaze_num[] = {1,11,0,25,4,8,19};
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

int birthday_num[] = {1,8,17,19,7,3,0,24};
void birthday(){
  //matrix.fillRect(0, 11, 32, 8, 0);
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
  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  matrix.setRotation(rot);

  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    // DO NOT CONTINUE if matrix setup encountered an error.
    for(;;);
  }
  //update_brightness(.5);
}


(matrix.color565(204,232,219),matrix.color565(193,212,227),matrix.color565(190,180,214),matrix.color565(250,218,226),matrix.color565(248,179,202),matrix.color565(204,151,193))
(matrix.color565(63,53,53),matrix.color565(169,92,74),matrix.color565(214,175,116),matrix.color565(135,163,100),matrix.color565(74,138,118),matrix.color565(61,80,112))


// LOOP - RUNS REPEATEDLY AFTER SETUP --------------------------------------
void loop() {
  update_digits();
  //scroll_color(scroll);
  //scroll_blue_white(scroll);
  //rand_yelp(scroll);
  //four_RBYW();
  enby();
  //matrix.fillRect(0, 0, 32, 16, 0);
  //cover_digits(pride[2]);
  //cover_background(0);
  //cover_digits(0);
  matrix.drawFastHLine(0, 10, 32, 0);
  //matrix.drawFastHLine(0, 0, 32, 0);
  date(matrix.color565(128, 128, 128));
  //birthday();
  matrix.show();
  delay(100);
}
