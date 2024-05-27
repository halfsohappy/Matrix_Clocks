#define BLACK_INK ink_swap(BLACK,BLACK,BLACK,BLACK)
#define WHITE_INK ink_swap(WHITE,WHITE,WHITE,WHITE)

void pattern_scroll_diagonal(){
  if(palette_size == 6){
    for(int i=0; i<48; i++){
      matrix.drawLine(0,i,i,0,palette[(abs(i - scroll) % palette_size)]);
    }
  }
  else{
    for(int i=0; i<24; i++){
      matrix.drawLine(0,2*i,2*i,0,palette[(abs(i - scroll) % palette_size)]);
      matrix.drawLine(0,2*i+1,2*i+1,0,palette[(abs(i - scroll) % palette_size)]);
    }
  }
}

void pattern_diagonal(){
  for(int i=0; i<48; i++){
    matrix.drawLine(0,i,i,0,palette[i % palette_size]);
  }
    for(int i=0; i<24; i++){
      matrix.drawLine(0,2*i,2*i,0,palette[i % palette_size]);
      matrix.drawLine(0,2*i+1,2*i+1,0,palette[i % palette_size]);
    }
}

void pattern_blocks(){ //best with no colon
  if(palette_size == 6){
    matrix.fillRect(0, 0, 6, 11, palette[0]);
    matrix.fillRect(6, 0, 5, 11, palette[1]);
    matrix.fillRect(11, 0, 5, 11, palette[2]);
    matrix.fillRect(16, 0, 5, 11, palette[3]);
    matrix.fillRect(21, 0, 5, 11, palette[4]);
    matrix.fillRect(26, 0, 6, 11, palette[5]);
  }
  if(palette_size == 4){
    for(int i = 0; i <4; i++){
      matrix.fillRect(i*8, 0, 8, 11, palette[i]);
    }
  }
}

void pattern_h_thin(){
  for(int i=0; i<12; i++){
    matrix.drawFastHLine(0,i,32,palette[i % palette_size]);
  }
}
void pattern_h_thick(){
  for(int i=0; i<palette_size; i++){
      matrix.fillRect(0, i*(12/palette_size), 32, 12/palette_size, palette[i]);
  }
}
void pattern_v_thin(){
  for(int i=0; i<32; i++){
    matrix.drawFastVLine(0,i,11,palette[i % palette_size]);
  }
}

void pattern_v_thick(){
  for(int i=0; i<palette_size; i++){
      matrix.fillRect(i*(12/palette_size), 0, 12/palette_size, 11, palette[i]);
  }
}

void pattern_random(){
  for(int x=0; x<32; x++){
    for(int y=0; y<16; y++){
    matrix.drawPixel(x,y,palette[int(random(1)*palette_size)]);
    }
  }
}

void switch_pattern(int pattern){
  switch (pattern){
    case 0:
      face_task.setCallback(&pattern_scroll_diagonal);
      face_task.setInterval(100);
    break;

    case 1:
      face_task.setCallback(&pattern_diagonal);
      face_task.setInterval(25);
    break;

    case 2:
      face_task.setCallback(&pattern_blocks);
      face_task.setInterval(25);
    break;

    case 3:
      face_task.setCallback(&pattern_h_thin);
      face_task.setInterval(100);
    break;

    case 4:
      face_task.setCallback(&pattern_h_thick);
      face_task.setInterval(25);
    break;

    case 5:
      face_task.setCallback(&pattern_v_thin);
      //showAll();
    break;

    case 6:
      face_task.setCallback(&pattern_h_thick);
    break;

    case 7:
      face_task.setCallback(&pattern_random);
      face_task.setInterval(100);
    break;
  }
}

//int palette_size;

void pal_swap(uint16_t zero, uint16_t one, uint16_t two, uint16_t three, uint16_t four = 0, uint16_t five = 0){
  palette[0] = zero;
  palette[1] = one;
  palette[2] = two;
  palette[3] = three;
  palette[4] = four;
  palette[5] = five;
}

void ink_swap(uint16_t zero, uint16_t one, uint16_t two, uint16_t three){
  ink_color[0] = zero;
  ink_color[1] = one;
  ink_color[2] = two;
  ink_color[3] = three;
}

void change_palette(){
  switch(current_palette){
    case 1://RAINBOW w/BLACK
      pal_swap(RED,ORANGE,YELLOW,GREEN,BLUE,PURPLE);
      palette_size = 6;
      BLACK_INK;
    break;

    case 2: //RAINBOW w/WHITE
      pal_swap(RED,ORANGE,YELLOW,GREEN,BLUE,PURPLE);
      palette_size = 6;
      WHITE_INK;
    break;

    case 3: //RBYW
      pal_swap(RED,BLUE,YELLOW,WHITE);
      palette_size = 4;
      BLACK_INK;
    break;

    case 4: //RGBW
      pal_swap(PURE_RED,PURE_GREEN,PURE_BLUE,WHITE);
      palette_size = 4;
      BLACK_INK;
    break;

    case 5: //CMYK
      pal_swap(CYAN, MAGENTA, YELLOW, BLACK);
      palette_size = 4;
      WHITE_INK;
    break;

    case 6: //funky pinkygreen
      pal_swap(matrix.color565(204,232,219),matrix.color565(193,212,227),matrix.color565(190,180,214),matrix.color565(250,218,226),matrix.color565(248,179,202),matrix.color565(204,151,193));
      palette_size = 6;
      BLACK_INK;
    break;

    case 7: //"wilderness" palette
      pal_swap(matrix.color565(63,53,53),matrix.color565(169,92,74),matrix.color565(214,175,116),matrix.color565(135,163,100),matrix.color565(74,138,118),matrix.color565(61,80,112));
      palette_size = 6;
      BLACK_INK;
    break;

    case 8: //Duke palette
      pal_swap(DUKE_BLUE, GRAY, WHITE, DUKE_BLUE);
      palette_size = 4;
      BLACK_INK;
    break;

    case 9: //purple yellow thing
      pal_swap(PURPLE, YELLOW, PURPLE, YELLOW);
      palette_size = 4;
      ink_swap(YELLOW, PURPLE, YELLOW, PURPLE);
    break;

    case 10: //orange cyan thing
      pal_swap(ORANGE, BLACK, BLUE, BLACK);
      palette_size = 4;
      ink_swap(BLUE, ORANGE, ORANGE, BLUE);
    break;

    case 11: //monochrome
      pal_swap(BLACK, WHITE, BLACK, WHITE);
      palette_size = 4;
      ink_swap(GRAY, GRAY, GRAY, GRAY);
    break;
  }
}

void change_pal_helper(){
  current_palette +=1;
  if(current_palette == 12){current_palette = 0;}
}
void change_pat_helper(){
  current_pattern +=1;
  if(current_pattern == 8){current_pattern = 0;}
}


// Simple set of patterns:
// -Diagonal rainbow scroll
// -Black and white Static
// -color block
// -inverse color block with different color
// -blue and white
// -one or two pretty contrasts
//set time: hold down button, then can change hour?





void disablePattern(){
  face_task.disable();
}