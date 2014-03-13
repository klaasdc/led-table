/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Basic animation with fading stars
 */

#define  NB_NEWSTARS  1
boolean starsRunning;

void initStars(){
  starsRunning = true;
}

void runStars(){
  initStars();
  clearTablePixels();
  leds.show();
  
  unsigned long prevStarsCreationTime = 0;
  unsigned long prevUpdateTime = 0;
  unsigned long curTime = 0;
  
  while(starsRunning){
    dimLeds(0.97);
    
    //Create new stars if enough time has passed since last time
    curTime = millis();
    if ((curTime - prevStarsCreationTime) > 1800){
      for (int i=0; i<NB_NEWSTARS; i++){
        boolean positionOk = false;
        int n = 0;
        //Generate random positions until valid
        while (!positionOk){
          n = random(FIELD_WIDTH*FIELD_HEIGHT);
          if (leds.getPixel(n) == 0)
            positionOk = true;
        }
//        //Get random color
//        if (random(2)==0)
//          leds.setPixel(n,YELLOW);
//        else
          leds.setPixel(n,WHITE);
      }
      prevStarsCreationTime = curTime;
    }
    
    leds.show();
    
    //Check input keys
    do{
      readInput();
      if (curControl == BTN_START){
        starsRunning = false;
        break;
      }
      curTime = millis();
      delay(10);
    } 
    while ((curTime - prevUpdateTime) <80);//Once enough time  has passed, proceed. The lower this number, the faster the game is
    prevUpdateTime = curTime;
  }
}
