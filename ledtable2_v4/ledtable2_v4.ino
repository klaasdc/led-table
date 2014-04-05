/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Main file with common methods and defines, such as button reading from USB controller
 * or setting pixels on the LED area
 */

//LED field size
#define  FIELD_WIDTH       12
#define  FIELD_HEIGHT      12
#define  ORIENTATION_HORIZONTAL //Rotation of table, uncomment to rotate field 90 degrees

//#define USE_OCTOWS2811 // Select either OCTOWS2811 or
#define USE_FAST_LED   // FAST_LED as library to control the LED strips

/*
 * Some defines used by the FAST_LED library
 */
#define FAST_LED_CHIPSET WS2801
#define FAST_LED_CHIPSET_WITH_CLOCK  // Comment if you use a chipset without clock
#define FAST_LED_DATA_PIN  7
#define FAST_LED_CLOCK_PIN 6

#if defined(USE_OCTOWS2811) && defined(USE_FAST_LED)
#error "Only one of USE_OCTOWS2811 and USE_FAST_LED can be defined"
#endif

#define  NUM_PIXELS    FIELD_WIDTH*FIELD_HEIGHT

/*
 * Define to disable the USB Input
 */
//#define DISABLE_USB

//#define USE_CONSOLE_OUTPUT //Uncomment if you want to be able to display the led array on the serial console (for debugging)
//#define USE_CONSOLE_INPUT //Uncomment if you want to be able to play the game from the console

//USB and Xbox gamepad library
#include <XBOXUSB.h>
#include <spi4teensy3.h>
USB Usb;
XBOXUSB Xbox(&Usb);

//Timer that periodically calls Usb.Task()
IntervalTimer usbTaskTimer;

/* *** LED color table *** */
#define  GREEN  0x00FF00
#define  RED    0xFF0000
#define  BLUE   0x0000FF
#define  YELLOW 0xFFFF00
#define  LBLUE  0x00FFFF
#define  PURPLE 0xFF00FF
#define  WHITE  0XFFFFFF
unsigned int colorLib[6] = {RED, GREEN, BLUE, YELLOW, LBLUE, PURPLE};

/* *** Game commonly used defines ** */
#define  DIR_UP    1
#define  DIR_DOWN  2
#define  DIR_LEFT  3
#define  DIR_RIGHT 4

/* *** USB controller button defines and input method *** */
#define  BTN_NONE  0
#define  BTN_UP    1
#define  BTN_DOWN  2
#define  BTN_LEFT  3
#define  BTN_RIGHT  4
#define  BTN_START  5

uint8_t curControl = BTN_NONE;
#ifndef USE_CONSOLE_INPUT
uint8_t prevControl = BTN_NONE;
void readInput(){
  curControl = BTN_NONE;
  if (Xbox.Xbox360Connected) {
    if (Xbox.getButtonPress(B) || Xbox.getButtonPress(RIGHT))
      curControl = BTN_RIGHT;
    else if (Xbox.getButtonPress(X) || Xbox.getButtonPress(LEFT))
      curControl = BTN_LEFT;
    else if (Xbox.getButtonPress(Y) || Xbox.getButtonPress(UP))
      curControl = BTN_UP;
    else if (Xbox.getButtonPress(A) || Xbox.getButtonPress(DOWN))
      curControl = BTN_DOWN;
    else if (Xbox.getButtonPress(START) || Xbox.getButtonPress(XBOX))
      curControl = BTN_START;
      
    if (curControl == prevControl)
      curControl = BTN_NONE;
    else
      prevControl = curControl;
  }
}
#else
void readInput(){
  curControl = BTN_NONE;
  if (Serial.available() > 0) {
    // read the incoming byte:
    uint8_t incomingByte = Serial.read();
    if (incomingByte == 27){
      incomingByte = Serial.read();//Should be 91 so that escape sequence is complete
      incomingByte = Serial.read();//This character determines actual key press
      switch(incomingByte){
      case 68:
        curControl = BTN_LEFT;
        break;
      case 67:
        curControl = BTN_RIGHT;
        break;
      case 65:
        curControl = BTN_UP;
        break;
      case 66:
        curControl = BTN_DOWN;
        break;
      }
    } 
    else if (incomingByte == 32){//Space bar pressed, reset active brick
      curControl = BTN_START;
    }
  }
}
#endif

//For debugging, allow field to be printed on serial console
#ifdef USE_CONSOLE_OUTPUT
int consolePixelArray[FIELD_WIDTH][FIELD_HEIGHT];
#define setTablePixelConsole(X,Y,C)  consolePixelArray[X][Y] = (C>0)
void outputTableToConsole(){
  Serial.write(27);       // escape command
  Serial.print("[2J");    // clear screen command
  Serial.write(27);       // escape command
  Serial.print("[H");     // cursor to home command
  uint8_t x,y;
  for (y=0;y<FIELD_WIDTH;y++){
    Serial.print("|");
    for (x=0;x<FIELD_HEIGHT;x++){
      if (consolePixelArray[x][y] == 1){
        Serial.print("X");
      } else {
        Serial.print(" ");
      }
    }
    Serial.println("|");
  }
  Serial.print("------------");
}
void clearTablePixelsConsole(){
  uint8_t x,y;
  for (x=0;x<FIELD_WIDTH;x++){
    for (y=0;y<FIELD_HEIGHT;y++){
      consolePixelArray[x][y] = 0;
    }
  }
}
#endif

/* *** pixel toggle helper method *** */
//#ifdef ORIENTATION_HORIZONTAL
//  #ifdef USE_CONSOLE_OUTPUT
//    #define  setTablePixel(PY,PX,PC)  leds.setPixel(PX%2 ? PX*FIELD_WIDTH + PY : PX*FIELD_WIDTH + ((FIELD_HEIGHT-1)-PY),PC); setTablePixelConsole(PY,PX,PC)
//  #else
//    #define  setTablePixel(PY,PX,PC)  leds.setPixel(PX%2 ? PX*FIELD_WIDTH + PY : PX*FIELD_WIDTH + ((FIELD_HEIGHT-1)-PY),PC)
//  #endif
//#else
//  #ifdef USE_CONSOLE_OUTPUT
//    #define  setTablePixel(PX,PY,PC)  leds.setPixel(PX%2 ? PX*FIELD_WIDTH + ((FIELD_HEIGHT-1)-PY) : PX*FIELD_WIDTH + PY,PC); setTablePixelConsole(PX,PY,PC)
//  #else
//    #define  setTablePixel(PX,PY,PC)  leds.setPixel(PX%2 ? PX*FIELD_WIDTH + ((FIELD_HEIGHT-1)-PY) : PX*FIELD_WIDTH + PY,PC)
//  #endif
//#endif

/*
 * OCTOWS2811 implementation 
 */
#ifdef USE_OCTOWS2811

//OctoWS2811 instance
#include <OctoWS2811.h>

const int ledsPerStrip = FIELD_HEIGHT*2;
DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

void initPixels(){
  leds.begin();
}

void setPixel(int n, int color){
  leds.setPixel(n, color);  
}

int getPixel(int n){
  return leds.getPixel(n);  
}

void showPixels(){
  leds.show(); 
}

#endif

/*
 * FAST_LED implementation 
 */
#ifdef USE_FAST_LED

#include "FastLED.h"

CRGB leds[NUM_PIXELS];

void initPixels(){
#ifdef FAST_LED_CHIPSET_WITH_CLOCK
  FastLED.addLeds<FAST_LED_CHIPSET, FAST_LED_DATA_PIN, FAST_LED_CLOCK_PIN, RGB>(leds, NUM_PIXELS);
#else
  FastLED.addLeds<FAST_LED_CHIPSET, FAST_LED_DATA_PIN, RGB>(leds, NUM_PIXELS);
#endif
}

void setPixel(int n, int color){
  leds[n] = CRGB(color);
}

int getPixel(int n){
  return (leds[n].r << 16) + (leds[n].g << 8) + leds[n].b;  
}

void showPixels(){
  FastLED.show();
}

#endif


void setTablePixel(int x, int y, int color){
  #ifdef ORIENTATION_HORIZONTAL
  setPixel(y%2 ? y*FIELD_WIDTH + x : y*FIELD_WIDTH + ((FIELD_HEIGHT-1)-x),color);
    #ifdef USE_CONSOLE_OUTPUT
      setTablePixelConsole(y,x,color);
    #endif
  #else
  setPixel(x%2 ? x*FIELD_WIDTH + ((FIELD_HEIGHT-1)-y) : x*FIELD_WIDTH + y,color);
    #ifdef USE_CONSOLE_OUTPUT
      setTablePixelConsole(x,y,color);
    #endif
  #endif
}

void clearTablePixels(){
  for (int n=0; n<FIELD_WIDTH*FIELD_HEIGHT; n++){
    setPixel(n,0);
  }
}

/* *** text helper methods *** */
#include "font.h"
uint8_t charBuffer[8][8];

void printText(char* text, unsigned int textLength, int xoffset, int yoffset, int color){
  uint8_t curLetterWidth = 0;
  int curX = xoffset;
  clearTablePixels();
  
  //Loop over all the letters in the string
  for (int i=0; i<textLength; i++){
    //Determine width of current letter and load its pixels in a buffer
    curLetterWidth = loadCharInBuffer(text[i]);
    //Loop until width of letter is reached
    for (int lx=0; lx<curLetterWidth; lx++){
      //Now copy column per column to field (as long as within the field
      if (curX>=FIELD_WIDTH){//If we are to far to the right, stop loop entirely
        break;
      } else if (curX>=0){//Draw pixels as soon as we are "inside" the drawing area
        for (int ly=0; ly<8; ly++){//Finally copy column
          setTablePixel(curX,yoffset+ly,charBuffer[lx][ly]*color);
        }
      }
      curX++;
    }
  }
  
  showPixels();
  #ifdef USE_CONSOLE_OUTPUT
  outputTableToConsole();
  #endif
}

//Load char in buffer and return width in pixels
uint8_t loadCharInBuffer(char letter){
  uint8_t* tmpCharPix;
  uint8_t tmpCharWidth;
  
  int letterIdx = (letter-32)*8;
  
  int x=0; int y=0;
  for (int idx=letterIdx; idx<letterIdx+8; idx++){
    for (int x=0; x<8; x++){
      charBuffer[x][y] = ((font[idx]) & (1<<(7-x)))>0;
    }
    y++;
  }
  
  tmpCharWidth = 8;
  return tmpCharWidth;
}


/* *********************************** */

void fadeOut(){
  //Select random fadeout animation
  int selection = random(3);
  
  switch(selection){
    case 0:
    case 1:
    {
      //Fade out by dimming all pixels
      for (int i=0; i<100; i++){
        dimLeds(0.97);
        showPixels();
        delay(10);
      }
      break;
    }
    case 2:
    {
      //Fade out by swiping from left to right with ruler
      const int ColumnDelay = 10;
      int curColumn = 0;
      for (int i=0; i<FIELD_WIDTH*ColumnDelay; i++){
        dimLeds(0.97);
        if (i%ColumnDelay==0){
          //Draw vertical line
          for (int y=0;y<FIELD_HEIGHT;y++){
            setTablePixel(curColumn, y, GREEN);
          }
          curColumn++;
        }
        showPixels();
        delay(5);
      }
      //Sweep complete, keep dimming leds for short time
      for (int i=0; i<100; i++){
        dimLeds(0.9);
        showPixels();
        delay(5);
      }
      break;
    }
  }
}

void dimLeds(float factor){
  //Reduce brightness of all LEDs, typical factor is 0.97
  for (int n=0; n<(FIELD_WIDTH*FIELD_HEIGHT); n++){
    int curColor = getPixel(n);
    //Derive the tree colors
    int r = ((curColor & 0xFF0000)>>16);
    int g = ((curColor & 0x00FF00)>>8);
    int b = (curColor & 0x0000FF);
    //Reduce brightness
    r = r*factor;
    g = g*factor;
    b = b*factor;
    //Pack into single variable again
    curColor = (r<<16) + (g<<8) + b;
    //Set led again
    setPixel(n,curColor);
  }
}


void setup(){
  Serial.begin(115200);
  //Wait for serial port to connect
  #ifdef USE_CONSOLE_OUTPUT
  while (!Serial);
  #endif

  //Initialise display
  initPixels();
  showPixels();

#ifdef DISABLE_USB
  //Init USB
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); //halt
  }
  
  //Init interrupt that calls Usb.task every 10 milliseconds
  usbTaskTimer.begin(UsbTask,10000);
#endif

  //Init random number generator
  randomSeed(millis());

  //Show menu
  mainLoop();
}

void UsbTask(void){
  Usb.Task();
}

void loop(){
}

