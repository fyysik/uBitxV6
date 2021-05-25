#include <Arduino.h>
#include <EEPROM.h>
#include "morse.h"
#include "ubitx.h"
#include "nano_gui.h"

/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * and the 16x2 LCD display.
 * The upper line of the display is constantly used to display frequency and status
 * of the radio. Occasionally, it is used to provide a two-line information that is 
 * quickly cleared up.
 */

#define BUTTON_SELECTED 1
#define UPPERLINE 2
#define MODELINE 42 ///45
#define COMMANDLINE 80
#define BANDLINE COMMANDLINE+26
#define CWSTATUSLINE 210 ///55
#define KPAD1 40
#define KPAD2 110
#define KPAD3 180

#define L630 450000L
#define U630 500000L
#define L160 1800000L
#define U160 2000000L
#define L80 3500000L
#define U80 4000000L
#define L60 5250000L
#define U60 5410000L
#define L40 7000000L
#define U40 7300000L
#define L30 10000000L
#define U30 10155000L
#define L20 14000000L
#define U20 14400000L
#define L17 18068000L
#define U17 18170000L
#define L15 21000000L
#define U15 21450000L
#define L13 24890000L
#define U13 24990000L
#define L11 26000000L
#define U11 27999999L
#define L10 28000000L
#define U10 30000000L



struct Button {
  int x, y, w, h;
  char *text;
  char *morse;
} btntmp;


#define MAX_BUTTONS 20
#define USB "USB"
#define LSB "LSB"
const struct Button btn_set[MAX_BUTTONS] PROGMEM = { 
//const struct Button  btn_set [] = {
  {0, UPPERLINE, 159, 36,  "VFOA", "A"},
  {160, UPPERLINE, 159, 36, "VFOB", "B"},
  
  {0, MODELINE, 60, 36,  "RIT", "R"},
  {64, MODELINE, 60, 36, "SSB", "S"},
  {128, MODELINE, 60, 36, "CW", "M"},
  {192, MODELINE, 60, 36, "SPL", "S"},
  {256, MODELINE, 60, 36, "FST", "F"},
  
  {0, BANDLINE, 60, 36, "160", "16"},
  {64, BANDLINE, 60, 36, "80", "8"},
  {128, BANDLINE, 60, 36, "60", "6"},
  {192, BANDLINE, 60, 36, "40", "4"},
  {256, BANDLINE, 60, 36, "30", "3"},
  {0, BANDLINE+45, 60, 36, "20", "2"},
  {64, BANDLINE+45, 60, 36, "17", "7"},
  {128, BANDLINE+45, 60, 36, "15", "5"},
  {192, BANDLINE+45, 60, 36, "11", "C"},
  {256, BANDLINE+45, 60, 36, "10", "1"},
  
  {0, CWSTATUSLINE-10, 60, 36, "WPM", "W"},
  {128, CWSTATUSLINE-10, 60, 36, "TON", "T"},

};

#define MAX_KEYS 20
const struct Button keypad[MAX_KEYS] PROGMEM = {   
  {0, KPAD1, 60, 50,  "1", "1"},
  {64, KPAD1, 60, 50, "2", "2"},
  {128, KPAD1, 60, 50, "3", "3"},
  {192, KPAD1, 60, 50,  "", ""},
  {256, KPAD1, 60, 50,  "<-", "B"},

  {0, KPAD2, 60, 50,  "4", "4"},
  {64, KPAD2, 60, 50,  "5", "5"},
  {128, KPAD2, 60, 50,  "6", "6"},
  {192, KPAD2, 60, 50,  "0", "0"},
  {256, KPAD2, 60, 50,  "ESC", "C"},

  {0, KPAD3, 60, 50,  "7", "7"},
  {64, KPAD3, 60, 50, "8", "8"},
  {128, KPAD3, 60, 50, "9", "9"},
  {192, KPAD3, 60, 50,  "", ""},
  {256, KPAD3, 60, 50,  "OK", "K"},
};
int max_buttons = sizeof(btn_set)/sizeof(struct Button);

boolean getButton(char *text, struct Button *b){
  for (int i = 0; i < max_buttons-1; i++){
    memcpy_P(b, btn_set + i, sizeof(struct Button));
    if (strstr(b->text, text)!=NULL){
      return true;
    }
  }
  return false;
}

int inBand(char *text){
  int inband=0;
  switch (atoi(text)) {
    case 160: if(L160 <= frequency && frequency <= U160) 
      inband=1; break;
    case 80:  if(L80 <= frequency && frequency <=U80) 
      inband=1; break;
    case 60:  if(L60 <= frequency && frequency <=U60) 
      inband=1; break;
    case 40:  if(L40 <= frequency && frequency <=U40) 
      inband=1;break;
    case 30:  if(L30 <= frequency &&  frequency <=U30) 
      inband=1; break;
    case 20:  if(L20 <= frequency && frequency <=U20) 
      inband=1; break;
    //case 17:  if(L17 <= frequency && frequency <=U17) inband=1; break;
//    case 15:  if(L15 <= frequency && frequency <=U15) 
//      inband=1; break;
    case 13: if(L13 <= frequency && frequency <= U13) 
      inband=1; break;
    case 11: if(L11 <= frequency && frequency <= U11) 
      inband=1; break;
    case 10: if(L10 <= frequency && frequency <= U10) 
      inband=1; break;
    default:; 
  }
  return 1;
}

/*
 * This formats the frequency given in f 
 */
void formatFreq(long f, char *buff) {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction

  memset(buff, 0, 10);
  memset(b, 0, sizeof(b));

  ultoa(f, b, DEC);

  //one mhz digit if less than 10 M, two digits if more
  if (f < 1000000l){
    //buff[0] = buff[1] = ' ';
    strncat(buff, b, 3);    
    strcat(buff, ".");
    strncat(buff, &b[3], 2);
  }
  else if (f < 10000000l){
    //buff[0] = ' ';
    strncat(buff, b, 4);    
    strcat(buff, ".");
    strncat(buff, &b[4], 2);
  }
  else {
    strncat(buff, b, 5);
    strcat(buff, ".");
    strncat(buff, &b[5], 2);    
  }
}

void drawCommandbar(char *text, int color=DISPLAY_WHITE, int background=DISPLAY_NAVY){
  displayFillrect(0,COMMANDLINE,320, 25, DISPLAY_NAVY);
  displayRawText(text, 0, COMMANDLINE, color, background);
}

/** A generic control to read variable values
*/
int getValueByKnob(int minimum, int maximum, int step_size,  int initial, char* prefix, char *postfix)
{
    int knob = 0;
    int knob_value;

    while (btnDown())
      active_delay(100);

    active_delay(200);
    knob_value = initial;
     
    strcpy(b, prefix);
    itoa(knob_value, c, 10);
    strcat(b, c);
    strcat(b, postfix);
    drawCommandbar(b);

    while(!btnDown() && digitalRead(PTT) == HIGH){

      knob = enc_read();
      if (knob != 0){
        if (knob_value > minimum && knob < 0)
          knob_value -= step_size;
        if (knob_value < maximum && knob > 0)
          knob_value += step_size;
          
        strcpy(b, prefix);
        itoa(knob_value, c, 10);
        strcat(b, c);
        strcat(b, postfix);
        drawCommandbar(b);
      }
      checkCAT();
    }
    drawCommandbar("");
   //displayFillrect(0,COMMANDLINE,280, 25, DISPLAY_NAVY); /// ???
   return knob_value;
}

void printCarrierFreq(unsigned long freq){

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(freq, b, DEC);
  
  strncat(c, b, 2);
  strcat(c, ".");
  strncat(c, &b[2], 3);
  strcat(c, ".");
  strncat(c, &b[5], 1);
  displayText(c, 110, 100, 100, 30, DISPLAY_CYAN, DISPLAY_NAVY, DISPLAY_NAVY);
}

void displayDialog(char *title, char *instructions){
  displayClear(DISPLAY_BLACK);
  displayRect(10,10,300,220, DISPLAY_WHITE);
  displayHline(20,COMMANDLINE,280,DISPLAY_WHITE);
  displayRect(12,12,296,216, DISPLAY_WHITE);
  displayRawText(title, 20, 20, DISPLAY_CYAN, DISPLAY_NAVY);
  displayRawText(instructions, 20, 200, DISPLAY_CYAN, DISPLAY_NAVY);
}




char vfoDisplay[12];

void displayVFO(int vfo){
  int x, y;
  int displayColor, displayBorder;

  if (vfo == VFO_A){
    getButton("VFOA", &btntmp);
    if (splitOn){
      if (vfoActive == VFO_A)
        strcpy(c, "R:");
      else 
        strcpy(c, "T:");
    }
    else  
      strcpy(c, "A:");
    if (vfoActive == VFO_A){
      formatFreq(frequency, c+2);
      displayColor = DISPLAY_WHITE;
      displayBorder = DISPLAY_BLACK;
    }else{
      formatFreq(vfoA, c+2);
      displayColor = DISPLAY_GREEN;
      displayBorder = DISPLAY_BLACK;      
    }
  }

  if (vfo == VFO_B){
    getButton("VFOB", &btntmp);

    if (splitOn){
      if (vfoActive == VFO_B)
        strcpy(c, "R:");
      else 
        strcpy(c, "T:");
    }
    else  
      strcpy(c, "B:");
    if (vfoActive == VFO_B){
      formatFreq(frequency, c+2);
      displayColor = DISPLAY_WHITE;
      displayBorder = DISPLAY_WHITE;
    } else {
      displayColor = DISPLAY_GREEN;
      displayBorder = DISPLAY_BLACK;
      formatFreq(vfoB, c+2);
    }
  }

  //displayText(c, btntmp.x, btntmp.y, btntmp.w, btntmp.h, displayColor, DISPLAY_BLACK, DISPLAY_DARKGREY);
  displayRawText(c, btntmp.x, btntmp.y, displayColor, DISPLAY_BLACK);
  strcpy(vfoDisplay, c);
}

void btnDraw(struct Button *b){
  if (!strcmp(b->text, "VFOA")){
    memset(vfoDisplay, 0, sizeof(vfoDisplay));
    displayVFO(VFO_A);
  }
  else if(!strcmp(b->text, "VFOB")){
    memset(vfoDisplay, 0, sizeof(vfoDisplay));    
    displayVFO(VFO_B);
  }
  else if ((!strcmp(b->text, "RIT") && ritOn == 1) || 
      //(!strcmp(b->text, USB) && isUSB == 1) || 
      //(!strcmp(b->text, LSB) && isUSB == 0) || 
      (!strcmp(b->text, "SPL") && splitOn == 1) ||
      (!strcmp(b->text, "CW") && cwMode == 1) ||
      (!strcmp(b->text, "FST") && fastOn == 1))
    displayText(b->text, b->x, b->y, b->w, b->h, DISPLAY_BLACK, DISPLAY_ORANGE, DISPLAY_DARKGREY);
  else if (strstr(b->text, "SB")  != NULL){
    if(isUSB)
      displayText(USB, b->x, b->y, b->w, b->h, DISPLAY_YELLOW, DISPLAY_BLUE, DISPLAY_DARKGREY);
    else
      displayText(LSB, b->x, b->y, b->w, b->h, DISPLAY_GREEN, DISPLAY_PINK, DISPLAY_DARKGREY);
  }
  else
    displayText(b->text, b->x, b->y, b->w, b->h, DISPLAY_GREEN, DISPLAY_BLACK, DISPLAY_DARKGREY);
}


void displayRIT(){
  displayFillrect(0,COMMANDLINE,320,20, DISPLAY_NAVY);
  if (ritOn){
    strcpy(c, "TX:");
    formatFreq(ritTxFrequency, c+3);
    if (vfoActive == VFO_A)
      displayText(c, 0, COMMANDLINE,159, 20, DISPLAY_WHITE, DISPLAY_NAVY, DISPLAY_NAVY);
    else
      displayText(c, 160, COMMANDLINE,159, 20, DISPLAY_WHITE, DISPLAY_NAVY, DISPLAY_NAVY);      
  }
  else {
    if (vfoActive == VFO_A)
      displayText("", 0, COMMANDLINE,159, 20, DISPLAY_WHITE, DISPLAY_NAVY, DISPLAY_NAVY);
    else
      displayText("", 160, COMMANDLINE,159, 20, DISPLAY_WHITE, DISPLAY_NAVY, DISPLAY_NAVY);
  }
}


void enterFreq(){
  //force the display to refresh everything
  //display all the buttons
  int f;
  displayClear(DISPLAY_BLACK);
  for (int i = 0; i < MAX_KEYS; i++){
    struct Button b;
    memcpy_P(&b, keypad + i, sizeof(struct Button));
    btnDraw(&b);
  }

  int cursor_pos = 0;
  memset(c, 0, sizeof(c));
  f = frequency / 1000l;

  while(1){

    checkCAT();
    if(!readTouch())
      continue;
      
    scaleTouch(&ts_point);

    int total = sizeof(btn_set)/sizeof(struct Button);
    for (int i = 0; i < MAX_KEYS; i++){
      struct Button b;
      memcpy_P(&b, keypad + i, sizeof(struct Button));

      int x2 = b.x + b.w;
      int y2 = b.y + b.h;
  
      if (b.x < ts_point.x && ts_point.x < x2 && 
        b.y < ts_point.y && ts_point.y < y2){
          if (!strcmp(b.text, "OK")){
            long f = atol(c);
            if(30000 >= f && f > 100){
              frequency = f * 1000l;
              setFrequency(frequency);
              if (vfoActive == VFO_A)
                vfoA = frequency;
              else
                vfoB = frequency;
              saveVFOs();
            }
            guiUpdate();
            return;
          }
          else if (!strcmp(b.text, "<-")){
            c[cursor_pos] = 0;
            if (cursor_pos > 0)
              cursor_pos--;      
            c[cursor_pos] = 0;
          }
          else if (!strcmp(b.text, "Esc")){
            guiUpdate();
            return;
          }
          else if('0' <= b.text[0] && b.text[0] <= '9'){
            c[cursor_pos++] = b.text[0];
            c[cursor_pos] = 0;
          }
        }
    } // end of the button scanning loop
    strcpy(b, c);
    strcat(b, " KHz");
    displayText(b, 0, UPPERLINE, 320, 30, DISPLAY_WHITE, DISPLAY_NAVY, DISPLAY_NAVY); ///42
    delay(300);
    while(readTouch())
      checkCAT();
  } // end of event loop : while(1)
  
}

void drawCWWPM(){
  displayFillrect(64, CWSTATUSLINE, 60, 39, DISPLAY_NAVY);
  int wpm = 1200/cwSpeed;    
  itoa(wpm,c, 10);
  strcpy(b, ": ");
  strcat(b, c);
  displayRawText(b, 64, CWSTATUSLINE, DISPLAY_CYAN, DISPLAY_NAVY);
  
 
}
void drawCWTone(){
  displayFillrect(194, CWSTATUSLINE, 650, 39, DISPLAY_NAVY);  
  itoa(sideTone, c, 10);
  strcpy(b, ": ");
  strcat(b, c);
  displayRawText(b, 194, CWSTATUSLINE, DISPLAY_CYAN, DISPLAY_NAVY);  
}


void drawTx(){
  if (inTx)
    displayText("TX", 280, COMMANDLINE, 37, 22, DISPLAY_BLACK, DISPLAY_ORANGE, DISPLAY_BLUE);  
  else
    displayFillrect(280, COMMANDLINE, 37, 22, DISPLAY_NAVY);
}
void drawStatusbar(){
  drawCWWPM();
  drawCWTone();
}

void guiUpdate(){

/*
  if (doingCAT)
    return;
*/
  // use the current frequency as the VFO frequency for the active VFO
  displayClear(DISPLAY_NAVY);

  memset(vfoDisplay, 0, 12);
  displayVFO(VFO_A);
  checkCAT();
  memset(vfoDisplay, 0, 12);  
  displayVFO(VFO_B);  

  checkCAT();
  displayRIT();
  checkCAT();

  //force the display to refresh everything
  //display all the buttons
  for (int i = 0; i < MAX_BUTTONS; i++){
    struct Button b;
    memcpy_P(&b, btn_set + i, sizeof(struct Button));
    btnDraw(&b);
    checkCAT();
  }
  drawStatusbar();
  checkCAT();  
}



// this builds up the top line of the display with frequency and mode
void updateDisplay() {
   displayVFO(vfoActive);    
}

int enc_prev_state = 3;

/**
 * The A7 And A6 are purely analog lines on the Arduino Nano
 * These need to be pulled up externally using two 10 K resistors
 * 
 * There are excellent pages on the Internet about how these encoders work
 * and how they should be used. We have elected to use the simplest way
 * to use these encoders without the complexity of interrupts etc to 
 * keep it understandable.
 * 
 * The enc_state returns a two-bit number such that each bit reflects the current
 * value of each of the two phases of the encoder
 * 
 * The enc_read returns the number of net pulses counted over 50 msecs. 
 * If the puluses are -ve, they were anti-clockwise, if they are +ve, the
 * were in the clockwise directions. Higher the pulses, greater the speed
 * at which the enccoder was spun
 */
/*
byte enc_state (void) {
    //Serial.print(digitalRead(ENC_A)); Serial.print(":");Serial.println(digitalRead(ENC_B));
    return (digitalRead(ENC_A) == 1 ? 1 : 0) + (digitalRead(ENC_B) == 1 ? 2: 0);
}

int enc_read(void) {
  int result = 0; 
  byte newState;
  int enc_speed = 0;
  
  long stop_by = millis() + 200;
  
  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state  
    
//    if (newState != enc_prev_state)
//      active_delay(20);
    
    if (enc_state() != newState || newState == enc_prev_state)
      continue; 
    //these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) || 
      (enc_prev_state == 2 && newState == 3) || 
      (enc_prev_state == 3 && newState == 1) || 
      (enc_prev_state == 1 && newState == 0)){
        result--;
      }
    //these transitions point o the enccoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) || 
      (enc_prev_state == 1 && newState == 3) || 
      (enc_prev_state == 3 && newState == 2) || 
      (enc_prev_state == 2 && newState == 0)){
        result++;
      }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    active_delay(1);
  }
  //if (result)
  //  Serial.println(result);
  return(result);
}
*/
void fastToggle(struct Button *b){
  if (fastOn == 0){
    fastOn =1;
  }
  else
    fastOn =0;
  btnDraw(b);
}

void ritToggle(struct Button *b){
  if (ritOn == 0){
    ritEnable(frequency);
  }
  else
    ritDisable();
  btnDraw(b);
  displayRIT();
}

void splitToggle(struct Button *b){

  if (splitOn)
    splitOn = 0;
  else
    splitOn = 1;

  btnDraw(b);

  //disable rit as well
  ritDisable();
  
  //struct Button b2;
  getButton("RIT", &btntmp);
  btnDraw(&btntmp);
  
  displayRIT();
  memset(vfoDisplay, 0, sizeof(vfoDisplay));
  displayVFO(VFO_A);
  memset(vfoDisplay, 0, sizeof(vfoDisplay));
  displayVFO(VFO_B);  
}

void vfoReset(){
  //Button b;
  if (vfoActive = VFO_A)
    vfoB = vfoA;
  else
    vfoA = vfoB;

  if (splitOn){
    getButton("SPL", &btntmp);
    splitToggle(&btntmp);
  }

  if (ritOn){
    getButton("RIT", &btntmp);
    ritToggle(&btntmp);
  }
  
  memset(vfoDisplay, 0, sizeof(vfoDisplay));
  displayVFO(VFO_A);
  memset(vfoDisplay, 0, sizeof(vfoDisplay));
  displayVFO(VFO_B);  

  saveVFOs();
}

void cwToggle(struct Button *b){
  if (cwMode == 0){
    cwMode = 1;
  }
  else
    cwMode = 0;

  setFrequency(frequency);
  btnDraw(b);
}

void sidebandToggle(char *ssb){
  //struct Button e;
  getButton("SB", &btntmp);
  if (!strcmp(ssb, LSB))
    isUSB = 0;
  else
    isUSB = 1;

  saveVFOs();
  btnDraw(&btntmp);
}


void redrawVFOs(){

    //struct Button b;
    ritDisable();
    getButton("RIT", &btntmp);
    btnDraw(&btntmp);
    displayRIT();
    memset(vfoDisplay, 0, sizeof(vfoDisplay));
    displayVFO(VFO_A);
    memset(vfoDisplay, 0, sizeof(vfoDisplay));
    displayVFO(VFO_B);
    //Serial.println(frequency);
    //draw the lsb/usb buttons, the sidebands might have changed
    sidebandToggle((isUSB == 0000000L)?"LSB":"USB");
}


void switchBand(long bandfreq){
  long offset =0;

 // Serial.println(frequency);
 //Serial.println(bandfreq);
 /* if (3500000l <= frequency && frequency <= 4000000l)
    offset = frequency - 3500000l;
  else if (24800000l <= frequency && frequency <= 25000000l)
    offset = frequency - 24800000l;
  else 
    offset = frequency % 1000000l; 

//  Serial.println(offset);*/
  //struct Button e;

  setFrequency(bandfreq + offset);
  //updateDisplay();
  
  sidebandToggle((bandfreq < 10000000L)?"LSB":"USB");
  saveVFOs();
}

int setCwSpeed(){
    int knob = 0;
    int wpm;

    wpm = 1200/cwSpeed;
    drawCommandbar("Set WPM, push TUNE");
    while (digitalRead(PTT) == HIGH && !btnDown())
    {


      knob = enc_read();

      if (knob > 0 && wpm < 200)
        wpm += 1;
      else if (knob < 0 && wpm > 2 )
         wpm -= 1;
      else
        continue; //don't update the frequency or the display
    cwSpeed = 1200/wpm;
      drawCWWPM();
      active_delay(20);
    };
    
    EEPROM.put(CW_SPEED, cwSpeed);
    drawCommandbar("");
    active_delay(500);
}

void setCwTone(){
  int knob = 0;
  int prev_sideTone;
     
  drawCommandbar("Set Tone, push TUNE");
 // sideTone = getValueByKnob(1, 100, 1,  sideTone, "CW: ", " Tone"); 
  //disable all clock 1 and clock 2 
  while (digitalRead(PTT) == HIGH && !btnDown())
  {


    knob = enc_read();

    if (knob > 0 && sideTone < 2000)
      sideTone += 10;
    else if (knob < 0 && sideTone > 100 )
      sideTone -= 10;
    else
      continue; //don't update the frequency or the display
    tone(CW_TONE, sideTone);
    //printLine2(b);
    drawCWTone();
    checkCAT();
    active_delay(20);
  };
  noTone(CW_TONE);
  //save the setting
  EEPROM.put(CW_SIDETONE, sideTone);
  drawCommandbar("");
  //displayFillrect(0, COMMANDLINE, 320, 22, DISPLAY_NAVY);
  //drawStatusbar();
//  printLine2("");  
//  updateDisplay();  
}

void doCommand(struct Button *b){
  
  if (!strcmp(b->text, "RIT"))
    ritToggle(b);
  else if (strstr(b->text, "SB")!=NULL) {
    Serial.print("doCommand toggle SB");
    sidebandToggle(isUSB?LSB:USB);
  }
  //else if (!strcmp(b->text, USB))
  //  sidebandToggle(USB);
  else if (!strcmp(b->text, "CW"))
    cwToggle(b);
  else if (!strcmp(b->text, "SPL"))
    splitToggle(b);
  else if (!strcmp(b->text, "VFOA")){
    if (vfoActive == VFO_A)
      enterFreq();//fastTune();
    else
      switchVFO(VFO_A);
  }
  else if (!strcmp(b->text, "VFOB")){
    if (vfoActive == VFO_B)
      enterFreq();//fastTune();
    else
      switchVFO(VFO_B);
  }
  else if (!strcmp(b->text, "A=B"))
    vfoReset();
  else if (!strcmp(b->text, "160"))
    switchBand(1800000l);
  else if (!strcmp(b->text, "80"))
    switchBand(3500000l);
  else if (!strcmp(b->text, "60"))
    switchBand(5250000l);
  else if (!strcmp(b->text, "40"))
    switchBand(7000000l);
  else if (!strcmp(b->text, "30"))
    switchBand(10000000l);
  else if (!strcmp(b->text, "20"))
    switchBand(14000000l);
  else if (!strcmp(b->text, "17"))
    switchBand(18000000l);
  else if (!strcmp(b->text, "15"))
    switchBand(21000000l);
  else if (!strcmp(b->text, "13"))
    switchBand(24800000l);
  else if (!strcmp(b->text, "11"))
    switchBand(27200000l);
  else if (!strcmp(b->text, "10"))
    switchBand(28000000l);  
  else if (!strcmp(b->text, "FST"))
    fastToggle(b);
    //enterFreq();
  else if (!strcmp(b->text, "WPM"))
    setCwSpeed();
  else if (!strcmp(b->text, "TON"))
    setCwTone();

}

void  checkTouch(){

  if (!readTouch())
    return;

  while(readTouch())
    checkCAT();
  scaleTouch(&ts_point);
 
  /* //debug code
  Serial.print(ts_point.x); Serial.print(' ');Serial.println(ts_point.y);
  */
  int total = sizeof(btn_set)/sizeof(struct Button);
  for (int i = 0; i < MAX_BUTTONS; i++){
    struct Button b;
    memcpy_P(&b, btn_set + i, sizeof(struct Button));

    int x2 = b.x + b.w;
    int y2 = b.y + b.h;

    if (b.x < ts_point.x && ts_point.x < x2 && 
      b.y < ts_point.y && ts_point.y < y2)
          doCommand(&b);
  }
}

//returns true if the button is pressed
int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}


void drawFocus(int ibtn, int color){
  struct Button b;

  memcpy_P(&b, btn_set + ibtn, sizeof(struct Button));
  displayRect(b.x, b.y, b.w, b.h, color);
}

void doCommands(){
  int select=0, i, prevButton, btnState;

  //wait for the button to be raised up
  while(btnDown())
    active_delay(50);
  active_delay(50);  //debounce

  menuOn = 2;

  while (menuOn){

    //check if the knob's button was pressed
    btnState = btnDown();
    if (btnState){
      struct Button b;
      memcpy_P(&b, btn_set + select/10, sizeof(struct Button));
    
      doCommand(&b);

      //unfocus the buttons
      drawFocus(select, DISPLAY_BLUE);
      if (vfoActive == VFO_A)
        drawFocus(0, DISPLAY_WHITE);
      else
        drawFocus(1, DISPLAY_WHITE);
        
      //wait for the button to be up and debounce
      while(btnDown())
        active_delay(100);
      active_delay(500);      
      return;
    }

    i = enc_read();
    
    if (i == 0){
      active_delay(50);
      continue;
    }
    
    if (i > 0){
      if (select + i < MAX_BUTTONS * 10)
        select += i;
    }
    if (i < 0 && select + i >= 0)
      select += i;      //caught ya, i is already -ve here, so you add it
    
    if (prevButton == select / 10)
      continue;
      
     //we are on a new button
     drawFocus(prevButton, DISPLAY_BLUE);
     drawFocus(select/10, DISPLAY_WHITE);
     prevButton = select/10;
  }
//  guiUpdate();

  //debounce the button
  while(btnDown())
    active_delay(50);
  active_delay(50);

  checkCAT();
}
