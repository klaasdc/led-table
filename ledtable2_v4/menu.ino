/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Menu system for the LED table
 */

#define MINSELECTION  1
#define MAXSELECTION  4
unsigned int curSelection = MINSELECTION;

#define TEXTSPEED  110

void mainLoop(void){
  char* curSelectionText;
  int curSelectionTextLength;
  unsigned long prevUpdateTime = 0;
  
  while(true){
    //Show menu system and wait for input
    clearTablePixels();
    switch (curSelection){
      case 1:
        curSelectionText = "1 Rainbow";
        curSelectionTextLength = 9;
        break;
      case 2:
        curSelectionText = "2 Tetris";
        curSelectionTextLength = 8;
        break;
      case 3:
        curSelectionText = "3 Snake";
        curSelectionTextLength = 7;
        break;
      case 4:
        curSelectionText = "4 Stars";
        curSelectionTextLength = 7;
        break;
    }
    
    boolean selectionChanged = false;
    boolean runSelection = false;
    //Scroll current selection text from right to left;
    for (int x=FIELD_WIDTH; x>-(curSelectionTextLength*8); x--){
      printText(curSelectionText, curSelectionTextLength, x, (FIELD_HEIGHT-8)/2, RED);
      //Read buttons
      unsigned long curTime;
      do{
        readInput();
        if (curControl != BTN_NONE){
          if (curControl == BTN_LEFT){
            curSelection--;
            selectionChanged = true;
          } else if (curControl == BTN_RIGHT){
            curSelection++;
            selectionChanged = true;
          } else if (curControl == BTN_START){
            runSelection = true;
          }
          
          checkSelectionRange();
        }
        curTime = millis();
      } while (((curTime - prevUpdateTime) < TEXTSPEED) && (curControl == BTN_NONE));//Once enough time  has passed, proceed
      prevUpdateTime = curTime;
      
      if (selectionChanged || runSelection)
        break;
    }
    
    //If we are here, it means a selection was changed or a game started, or user did nothing
    if (selectionChanged){
      //For now, do nothing
    } else if (runSelection){//Start selected game
      switch (curSelection){
        case 1:
          runRainbow();
          break;
        case 2:
          runTetris();
          break;
        case 3:
          runSnake();
          break;
        case 4:
          runStars();
          break;
      }
    } else {
      //If we are here, no action was taken by the user, so we will move to the next selection automatically
      curSelection++;
      checkSelectionRange();
    }
  }
}

void checkSelectionRange(){
  if (curSelection>MAXSELECTION){
    curSelection = MINSELECTION;
  } else if (curSelection<MINSELECTION){
    curSelection = MAXSELECTION;
  }
}
