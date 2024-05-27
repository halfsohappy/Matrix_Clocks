#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
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


  //MATRIX PINS:
  // uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
  // uint8_t addrPins[] = {17, 18, 19, 20, 21};
  // uint8_t clockPin   = 14;
  // uint8_t latchPin   = 15;
  // uint8_t oePin      = 16;
  // int rot = 2;

  // //RP2040FEATHER PINS:
  uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
  uint8_t addrPins[] = {25, 24, 29, 28};
  uint8_t clockPin   = 13;
  uint8_t latchPin   = 1;
  uint8_t oePin      = 0;
  int rot = 0;

Adafruit_Protomatter matrix(
  32,          // Width of matrix (or matrix chain) in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  3, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  false      // No double-buffering here (see "doublebuffer" example)
);

uint16_t colors[] = {
  matrix.color565(255,0,0),//Red
  matrix.color565(253, 152, 0),//Orange
  matrix.color565(255, 255, 0),//Yellow
  matrix.color565(51, 254, 0),//Green
  matrix.color565(0, 151, 253),//Blue
  matrix.color565(102, 51, 253),//Purple
  0,//Black
  65535,//White
  matrix.color565(72, 72, 72),//lightish gray
  matrix.color565(28, 33, 168),//duke blue
  matrix.color565(0,255,255),//Cyan
  matrix.color565(255,0,255),//Magenta
};

uint16_t base_pixels[16][32];

uint16_t top_pixels[16][32];


//RTC DECLARATIONS AND FUNCTIONS
RTC_DS3231 rtc;
DateTime now;

int time_hhmm[4] = {3, 1, 4, 1};

int months[12][3] = {{9,0,13}, {5,4,1}, {12,0,17}, {0,15,17}, {12,0,24}, {9,20,13}, {9,20,11}, {0,20,6}, {18,4,15}, {14,2,19}, {13,14,21}, {3,4,2}};
bool small_num[10][5][3] = {
  {
  {1,1,1},
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,1}},
  {//0
  {1,1,0},
  {0,1,0},
  {0,1,0},
  {0,1,0},
  {1,1,1}},
  {//1
  {1,1,1},
  {0,0,1},
  {0,1,0},
  {1,0,0},
  {1,1,1}},
  {//2
  {1,1,1},
  {0,0,1},
  {1,1,1},
  {0,0,1},
  {1,1,1}},
  {//3
  {1,0,1},
  {1,0,1},
  {1,1,1},
  {0,0,1},
  {0,0,1}},
  {//4
  {1,1,1},
  {1,0,0},
  {1,1,1},
  {0,0,1},
  {1,1,1}},
  {//5
  {1,1,1},
  {1,0,0},
  {1,1,1},
  {1,0,1},
  {1,1,1}},
  {//6
  {1,1,1},
  {0,0,1},
  {0,0,1},
  {0,0,1},
  {0,0,1}},
  {//7
  {1,1,1},
  {1,0,1},
  {1,1,1},
  {1,0,1},
  {1,1,1}},
  {//8
  {1,1,1},
  {1,0,1},
  {1,1,1},
  {0,0,1},
  {0,0,1}} //9
  };

bool letters[26][5][3] = {
  {
  {1,1,1},
  {1,0,1},
  {1,1,1},
  {1,0,1},
  {1,0,1}},
  {//A
  {1,1,1},
  {1,0,1},
  {1,1,0},
  {1,0,1},
  {1,1,1}},
  {//B
  {1,1,1},
  {1,0,0},
  {1,0,0},
  {1,0,0},
  {1,1,1}},
  {//C
  {1,1,0},
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,0}},
  {//D
  {1,1,1},
  {1,0,0},
  {1,1,1},
  {1,0,0},
  {1,1,1}},
  {//E
  {1,1,1},
  {1,0,0},
  {1,1,1},
  {1,0,0},
  {1,0,0}},
  {//F
  {1,1,1},
  {1,0,0},
  {1,0,1},
  {1,0,1},
  {1,1,1}},
  {//G
  {1,0,1},
  {1,0,1},
  {1,1,1},
  {1,0,1},
  {1,0,1}},
  {//H
  {1,1,1},
  {0,1,0},
  {0,1,0},
  {0,1,0},
  {1,1,1}},
  {//I
  {0,1,1},
  {0,0,1},
  {0,0,1},
  {1,0,1},
  {1,1,1}},
  {//J
  {1,0,1},
  {1,0,1},
  {1,1,0},
  {1,0,1},
  {1,0,1}},
  {//K
  {1,0,0},
  {1,0,0},
  {1,0,0},
  {1,0,0},
  {1,1,1}},
  {//L
  {1,0,1},
  {1,1,1},
  {1,0,1},
  {1,0,1},
  {1,0,1}},
  {//M
  {1,1,1},
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,0,1}},
  {//N
  {1,1,1},
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,1}},
  {//O
  {1,1,1},
  {1,0,1},
  {1,1,1},
  {1,0,0},
  {1,0,0}},
  {//P
  {1,1,1},
  {1,0,1},
  {1,0,1},
  {1,1,1},
  {0,0,1}},
  {//Q
  {1,1,1},
  {1,0,1},
  {1,1,0},
  {1,0,1},
  {1,0,1}},
  {//R
  {0,1,1},
  {1,0,0},
  {0,1,0},
  {0,0,1},
  {1,1,0}},
  {//S
  {1,1,1},
  {0,1,0},
  {0,1,0},
  {0,1,0},
  {0,1,0}},
  {//T
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,1}},
  {//U
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,0},
  {1,0,0}},
  {//V
  {1,0,1},
  {1,0,1},
  {1,0,1},
  {1,1,1},
  {1,0,1}},
  {//W
  {1,0,1},
  {1,0,1},
  {0,1,0},
  {1,0,1},
  {1,0,1}},
  {//X
  {1,0,1},
  {1,0,1},
  {0,1,0},
  {0,1,0},
  {0,1,0}},
  {//Y
  {1,1,1},
  {0,0,1},
  {0,1,0},
  {1,0,0}, 
  {1,1,1}}
  //Z
  };

bool num[10][10][8] = {
  //zero:
  {
  {0,0,1,1,1,0,0,0},
  {0,1,1,0,1,1,0,0},
  {1,1,0,0,0,1,1,0},
  {1,1,0,0,0,1,1,0},
  {1,1,0,1,0,1,1,0},
  {1,1,0,1,0,1,1,0},
  {1,1,0,0,0,1,1,0},
  {1,1,0,0,0,1,1,0},
  {0,1,1,0,1,1,0,0},
  {0,0,1,1,1,0,0,0}
  },
  //one:
  {
  {0,0,0,1,1,0,0,0},
  {0,0,1,1,1,0,0,0},
  {0,1,1,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,0,1,1,0,0,0},
  {0,1,1,1,1,1,1,0}
  },
  //two:
  {
    {0,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,1,1,0,0},
    {0,0,0,1,1,0,0,0},
    {0,0,1,1,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {1,1,0,0,0,0,0,0},
    {1,1,0,0,0,1,1,0},
    {1,1,1,1,1,1,1,0}
  },
  //three:
  {
    {0,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,1,1,1,1,0,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,0,0}
  },
  //four:
  {
  {0,0,0,0,1,1,0,0},
  {0,0,0,1,1,1,0,0},
  {0,0,1,1,1,1,0,0},
  {0,1,1,0,1,1,0,0},
  {1,1,0,0,1,1,0,0},
  {1,1,1,1,1,1,1,0},
  {0,0,0,0,1,1,0,0},
  {0,0,0,0,1,1,0,0},
  {0,0,0,0,1,1,0,0},
  {0,0,0,1,1,1,1,0}
  },
  //five:
  {
    {1,1,1,1,1,1,1,0},
    {1,1,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,0,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,0,0}
  },
  //six:
  {
    {0,0,1,1,1,0,0,0},
    {0,1,1,0,0,0,0,0},
    {1,1,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,0,0}
  },
  //seven:
  {
  {1,1,1,1,1,1,1,0},
  {1,1,0,0,0,1,1,0},
  {0,0,0,0,0,1,1,0},
  {0,0,0,0,0,1,1,0},
  {0,0,0,0,1,1,0,0},
  {0,0,0,1,1,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,1,1,0,0,0,0},
  {0,0,1,1,0,0,0,0}
  },
  //eight:
  {
    {0,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,0,0}
  },
  //nine:
  {
    {0,1,1,1,1,1,0,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,0,1,1,0},
    {0,0,0,0,1,1,0,0},
    {0,1,1,1,1,0,0,0}
  }
};


void refill_base(){
  for(int x = 0; x<32; x++){
    for(int y = 0; y<16; y++){
      matrix.drawPixel(x, y, base_pixels[y][x]);
    }
  }
}

void draw_digit(int start_x, int value){
  for(int y = 0; y < 10; y++){
    for(int x = 0; x < start_x + 8; x++){
      if(num[value][y][x]){matrix.drawPixel(x + start_x, y, top_pixels[y][x+start_x]);}
    }
  }
}

void draw_small(int start_x, int start_y, int value, bool letter){
  bool condition;
  for(int y = 0; y < 5; y++){
    for(int x = 0; x < 3; x++){
      condition = letter ? letters[value][y][x] : small_num[value][y][x];
      if(condition){matrix.drawPixel(x + start_x, y + start_y, top_pixels[y+start_y][x+start_x]);}
    }
  }
}

void draw_date(int start_x, int month[3], int day){
  int date[5] = {month[0], month[1], month[2], day%10, day-day%10};
  draw_small(start_x, 11, date[0], true);
  draw_small(start_x+4, 11, date[1], true);
  draw_small(start_x+8, 11, date[2], true);
  draw_small(start_x+12, 11, date[3], false);
  draw_small(start_x+16, 11, date[4], false);
}


void draw_array(uint8_t start_x, uint8_t start_y, bool value[], uint8_t wide, uint8_t tall){
int coord = 0;
  for(int y=0; y<tall; y++){
    for(int x = 0; x<wide; x++){
      if(value[coord]){matrix.drawPixel(x + start_x, y + start_y, top_pixels[y+start_y][x+start_x]);}
      coord++;
    }
  }
}

bool colon_array[] =
  {
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
typedef struct link{
  link* next;
  link* past;
  uint16_t color;
} link;

link* current = (link*) malloc(sizeof(link)*1);
link* head = current;
link* minus_one = NULL;

link* rainbow_link_array[6] ={head, head->next, head->next->next, head->next->next->next, head->next->next->next->next, head->next->next->next->next->next};

void rainbow_link_setup(){
  for(int i = 0; i < 5; i++){
    current->next = (link*) malloc(sizeof(link)*1);
    current->color = colors[i];
    minus_one = current;
    current = current->next;
    current->past = minus_one;
  }
  current->color = colors[5];
  current->next = head;
  head->past = current;
}

void diagonal_rainbow(int start){
  link* lo_current = rainbow_link_array[start];
  for(int y = 0; y<16; y++){
    for(int x = 0; x<32; x++){
      base_pixels[y][x] = lo_current->color;
      lo_current = lo_current->next;
    }
  lo_current = lo_current->past;//correct for ineven division
  }
}

void RBYW(){
  for(int y = 0; y<10; y++){
    for(int x = 0; x<32; x++){
      base_pixels[y][x] = (x<8) ? RED : (x<16) ? BLUE : (x<24) ? YELLOW : WHITE;
    }
  }
}

void duke_checker(){
  for(int x = 0; x<32; x++){
    bool vert[] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1};
    for(int y = 0; y<16; y++){
      base_pixels[y][x] = vert[y] ? DUKE_BLUE : WHITE;
    }
  }
}


//patterns:
// monochrome
// rainbow 
// color block 
// duke checkerboard
// one or two fun color combos


void setup() {
  Serial.begin(9600);
    
  rtc.begin();
  delay(100);
  now = rtc.now();

  ProtomatterStatus status = matrix.begin();
  matrix.setRotation(rot);

  // draw_date(10, months[now.month()], now.day());
  // draw_array(14, 0, (bool*)colon_array, 3, 7);
}

void loop() {
  now = rtc.now();
  matrix.show();
  duke_checker();
  delay(500);
  refill_base();
  // put your main code here, to run repeatedly:

}
