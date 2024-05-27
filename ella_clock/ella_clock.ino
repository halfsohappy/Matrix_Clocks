//import libraries
#include <Adafruit_Protomatter.h>
#include <Adafruit_GFX.h>
#include "RTClib.h"
#include "font_array.h"
#include <TaskScheduler.h>
#include <TaskSchedulerDeclarations.h>
#include <TaskSchedulerSleepMethods.h>
//color definitions
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
#define PURE_RED matrix.color565(255,0,0)
#define PURE_GREEN matrix.color565(0,255,0)
#define PURE_BLUE matrix.color565(0,0,255)




//find second sunday in march, if after, go forward
//find first sunday of november, if after, don't
bool ink;


  //MATRIX PINS:
  uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
  uint8_t addrPins[] = {17, 18, 19, 20, 21};
  uint8_t clockPin   = 14;
  uint8_t latchPin   = 15;
  uint8_t oePin      = 16;
  int rot = 2;

  // //RP2040FEATHER PINS:
  // uint8_t rgbPins[]  = {8, 7, 9, 11, 10, 12};
  // uint8_t addrPins[] = {25, 24, 29, 28};
  // uint8_t clockPin   = 13;
  // uint8_t latchPin   = 1;
  // uint8_t oePin      = 0;
  // int rot = 0;

//RTC DECLARATIONS AND FUNCTIONS
RTC_DS3231 rtc;
DateTime now;

//array holding digits related to time
int digits[] = {1,0,2,3};
//array holding 3 ints related to chars of month name and 2 ints of date number
int date_array[] = {0,0,0,0,0};

//function used to collect current time and date
void access_rtc(){
  now = rtc.now();
  //updating time
  if(now.twelveHour() < 10){digits[0] = 0; digits[1] = now.twelveHour();}
  else{digits[0] = 1; digits[1] = now.twelveHour()-10;}
  digits[2] = (now.minute() / 10);
  digits[3] = now.minute() % 10;
  //updating date
  for(int letter = 0; letter <3; letter++){date_array[letter] = months[now.month()-1][letter];}
  date_array[3] = (now.day() / 10);
  date_array[4] = now.day() % 10;
}

DateTime calc_dst_start(){
  now = rtc.now();
  DateTime this_day;
  int day_of_month = 1;
  for(int num_sunday = 0; num_sunday < 2;){
    this_day = DateTime(now.year(), 3, day_of_month);
    if(!this_day.dayOfTheWeek()){ //if it's sunday
      num_sunday++;
    }
    else{
      day_of_month++;
    }
  }
  return this_day;
}

DateTime calc_dst_end(){
  now = rtc.now();
  int day_of_month = 1;
  DateTime this_day;
  for(int num_sunday = 0; num_sunday < 1;){
    this_day = DateTime(now.year(), 11, day_of_month);
    if(!this_day.dayOfTheWeek()){ //if it's sunday
      num_sunday++;
    }
    else{
      day_of_month++;
    }
  }
  return this_day;
}

bool check_dst(){
return (calc_dst_start() <= rtc.now()) && (rtc.now() <= calc_dst_end());
}


//MATRIX CONSTRUCTOR
Adafruit_Protomatter matrix(
  32,          // Width of matrix (or matrix chain) in pixels
  4,           // Bit depth, 1-6
  1, rgbPins,  // # of matrix chains, array of 6 RGB pins for each
  3, addrPins, // # of address pins (height is inferred), array of pins
  clockPin, latchPin, oePin, // Other matrix control pins
  false      // No double-buffering here (see "doublebuffer" example)
);

//CREATE COLOR ARRAYS FOR CONVENIENCE
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



uint16_t palette[6];
uint16_t ink_color[4] = {WHITE, WHITE, WHITE, WHITE};
int palette_size = 6;
int current_palette = 0;
int current_pattern = 0;

//SCHEDULER DECLARATIONS
Scheduler face_scheduler;
Task face_task(100, -1);
Task update_digits_task(50, -1, &access_rtc);
//Task check_minute_task(5000, -1, &check_minute);

int scroll = 0;

#include "face_task_list.h"

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

void display_time(bool colon, bool bg){ //if bg is true, cover the background with ink
  if(!colon || digits[0]){
    for(int dig = 0; dig < 4; dig++){
      for(int i = 0; i<80; i++){
        if((num[digits[dig]][i] && !bg) || (!num[digits[dig]][i] && bg)){
          matrix.drawPixel((trans_x(i) + dig*8), trans_y(i), ink_color[dig]);
        }
      }
    } return;
  }

  if(colon && bg){
    matrix.fillRect(0, 0, 4, 10, ink);
    for(int i = 0; i<80; i++){
      if(!num[digits[1]][i]){
        matrix.drawPixel(trans_x(i) + 4, trans_y(i), ink_color[1]);
      }
    }

    matrix.fillRect(12, 0, 4, 2, ink);
    matrix.fillRect(12, 8, 4, 2, ink);
    matrix.fillRect(12, 0, 1, 10, ink);
    matrix.fillRect(12, 4, 4, 2, ink);
    matrix.fillRect(15, 0, 1, 10, ink);

    for(int i = 0; i<80; i++){
      if(!num[digits[2]][i]){
        matrix.drawPixel((trans_x(i) + 16), trans_y(i), ink_color[2]);
      }
      if(!num[digits[3]][i]){
        matrix.drawPixel((trans_x(i) + 24), trans_y(i), ink_color[3]);
      }
    } return;
  }

  if(colon && !bg){
    for(int i = 0; i<80; i++){
      if(num[digits[1]][i]){
          matrix.drawPixel(trans_x(i) + 4, trans_y(i), ink);
      }
    }

    matrix.fillRect(13, 2, 2, 2, ink);
    matrix.fillRect(13, 6, 2, 2, ink);

    for(int i = 0; i<80; i++){
      if(num[digits[2]][i]){
        matrix.drawPixel((trans_x(i) + 16), trans_y(i), ink);
      }
      if(num[digits[3]][i]){
        matrix.drawPixel((trans_x(i) + 24), trans_y(i), ink);
      }
    }
  }
}

// void date(bool bg){ //if bg is true, cover the background with ink
//   if(!bg){
//     for(int place = 0; place<3; place++){
//       for(int i = 0; i<15; i++){
//         if(letters[date_places[place]][i]){
//           matrix.drawPixel(10+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[2]);
//         }
//       }
//     }
//     for(int place = 3; place<5; place++){
//       for(int i = 0; i<15; i++){
//         if(small_num[date_places[place]][i]){
//           matrix.drawPixel(11+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[3]);
//         }
//       }
//     }
//   }
//   else{
//     for(int place = 0; place<3; place++){
//       for(int i = 0; i<15; i++){
//         if(letters[date_places[place]][i]){
//           matrix.drawPixel(10+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[2]);
//         }
//       }
//     }
//     for(int place = 3; place<5; place++){
//       for(int i = 0; i<15; i++){
//         if(small_num[date_places[place]][i]){
//           matrix.drawPixel(11+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[3]);
//         }
//       }
//     }
//   }
// }

int blaze_num[] = {1,11,0,25,4,8,19};

// void blaze_it(){
//   for(int place = 0; place<5; place++){
//     for(int i = 0; i<15; i++){
//       if(letters[blaze_num[place]][i]){
//         matrix.drawPixel(1+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, pride[0]);
//       }
//     }
//   }
//   for(int place = 5; place<7; place++){
//     for(int i = 0; i<15; i++){
//       if(letters[blaze_num[place]][i]){
//         matrix.drawPixel(2+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, pride[0]);
//       }
//     }
//   }
//}

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
 matrix.drawPixel(4,4,0xffff);

  matrix.show();
  delay(100);


    //   for(int place = 0; place<3; place++){
    //   for(int i = 0; i<15; i++){
    //     if(letters[date_array[place]][i]){
    //       matrix.drawPixel(10+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[2]);
    //     }
    //   }
    // }
    // for(int place = 3; place<5; place++){
    //   for(int i = 0; i<15; i++){
    //     if(small_num[date_array[place]][i]){
    //       matrix.drawPixel(11+ trans_x_smol(i)+ place*4, trans_y_smol(i)+11, ink_color[3]);
    //     }
    //   }
    // }


  //face_scheduler.addTask(face_task);
  //face_scheduler.addTask(update_digits_task);
}

// LOOP - RUNS REPEATEDLY AFTER SETUP --------------------------------------
void loop() {

 // face_scheduler.execute();
  scroll++;
  if(scroll == 100){scroll = 0;}

}
