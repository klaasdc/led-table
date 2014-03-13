/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Main code for the Tetris game
 */

uint16_t brickSpeed;
uint8_t nbRowsThisLevel;
uint16_t nbRowsTotal;

boolean tetrisGameOver;

void tetrisInit(){
  clearField();
  brickSpeed = INIT_SPEED;
  nbRowsThisLevel = 0;
  nbRowsTotal = 0;
  tetrisGameOver = false;
  
  newActiveBrick();
}

boolean tetrisRunning = false;
void runTetris(void){
  tetrisInit();
  
  unsigned long prevUpdateTime = 0;
  
  tetrisRunning = true;
  while(tetrisRunning){
    unsigned long curTime;
    do{
      //Usb.Task();
      readInput();
      if (curControl != BTN_NONE){
        playerControlActiveBrick();
        printField();
      }
      if (tetrisGameOver) break;
 
      curTime = millis();
      //delay(10);//If time too small, things break in print routine for some reason
    } while ((curTime - prevUpdateTime) < brickSpeed);//Once enough time  has passed, proceed. The lower this number, the faster the game is
    prevUpdateTime = curTime;
  
    if (tetrisGameOver){
      //fadeOut();
      char buf[4];
      int len = sprintf(buf, "%i", nbRowsTotal);
      
      scrollTextBlocked(buf,len,WHITE);
      
      //Disable loop and exit to main menu of led table
      tetrisRunning = false;
      break;
    }
  
    //If brick is still "on the loose", then move it down by one
    if (activeBrick.enabled){
      shiftActiveBrick(DIR_DOWN);
    } else {
      //Active brick has "crashed", check for full lines
      //and create new brick at top of field
      checkFullLines();
      newActiveBrick();
      prevUpdateTime = millis();//Reset update time to avoid brick dropping two spaces
    }
    printField();
  }
  
  fadeOut();
}

void playerControlActiveBrick(){
  switch(curControl){
    case BTN_LEFT:
      shiftActiveBrick(DIR_LEFT);
      break;
    case BTN_RIGHT:
      shiftActiveBrick(DIR_RIGHT);
      break;
    case BTN_DOWN:
      shiftActiveBrick(DIR_DOWN);
      break;
    case BTN_UP:
      rotateActiveBrick();
      break;
    case BTN_START:
      tetrisRunning = false;
      break;
  }
}

void printField(){
  int x,y;
  for (x=0;x<FIELD_WIDTH;x++){
    for (y=0;y<FIELD_HEIGHT;y++){
      uint8_t activeBrickPix = 0;
      if (activeBrick.enabled){//Only draw brick if it is enabled
        //Now check if brick is "in view"
        if ((x>=activeBrick.xpos) && (x<(activeBrick.xpos+(activeBrick.siz)))
            && (y>=activeBrick.ypos) && (y<(activeBrick.ypos+(activeBrick.siz)))){
          activeBrickPix = (activeBrick.pix)[x-activeBrick.xpos][y-activeBrick.ypos];
        }
      }
      if (field.pix[x][y] == 1){
        setTablePixel(x,y, field.color[x][y]);
      } else if (activeBrickPix == 1){
        setTablePixel(x,y, activeBrick.color);
      } else {
        setTablePixel(x,y, 0x000000);
      }
    }
  }
  leds.show();
  #ifdef USE_CONSOLE_OUTPUT
  outputTableToConsole();
  #endif
}

/* *** Game functions *** */

void newActiveBrick(){
//  uint8_t selectedBrick = 3;
  uint8_t selectedBrick = random(7);
  uint8_t selectedColor = random(6);

  //Set properties of brick
  activeBrick.siz = brickLib[selectedBrick].siz;
  activeBrick.yOffset = brickLib[selectedBrick].yOffset;
  activeBrick.xpos = FIELD_WIDTH/2 - activeBrick.siz/2;
  activeBrick.ypos = BRICKOFFSET-activeBrick.yOffset;
  activeBrick.enabled = true;
  
  //Set color of brick
  activeBrick.color = colorLib[selectedColor];
  
  //Copy pix array of selected Brick
  uint8_t x,y;
  for (y=0;y<MAX_BRICK_SIZE;y++){
    for (x=0;x<MAX_BRICK_SIZE;x++){
      activeBrick.pix[x][y] = (brickLib[selectedBrick]).pix[x][y];
    }
  }
  
  //Check collision, if already, then game is over
  if (checkFieldCollision(&activeBrick)){
    tetrisGameOver = true;
  }
}

//Check collision between bricks in the field and the specified brick
boolean checkFieldCollision(struct Brick* brick){
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      fx = (*brick).xpos + bx;
      fy = (*brick).ypos + by;
      if (( (*brick).pix[bx][by] == 1) 
            && ( field.pix[fx][fy] == 1)){
        return true;
      }
    }
  }
  return false;
}

//Check collision between specified brick and all sides of the playing field
boolean checkSidesCollision(struct Brick* brick){
  //Check vertical collision with sides of field
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      if ( (*brick).pix[bx][by] == 1){
        fx = (*brick).xpos + bx;//Determine actual position in the field of the current pix of the brick
        fy = (*brick).ypos + by;
        if (fx<0 || fx>=FIELD_WIDTH){
          return true;
        }
      }
    }
  }
  return false;
}

Brick tmpBrick;

void rotateActiveBrick(){
  //Copy active brick pix array to temporary pix array
  uint8_t x,y;
  for (y=0;y<MAX_BRICK_SIZE;y++){
    for (x=0;x<MAX_BRICK_SIZE;x++){
      tmpBrick.pix[x][y] = activeBrick.pix[x][y];
    }
  }
  tmpBrick.xpos = activeBrick.xpos;
  tmpBrick.ypos = activeBrick.ypos;
  tmpBrick.siz = activeBrick.siz;
  
  //Depending on size of the active brick, we will rotate differently
  if (activeBrick.siz == 3){
    //Perform rotation around center pix
    tmpBrick.pix[0][0] = activeBrick.pix[0][2];
    tmpBrick.pix[0][1] = activeBrick.pix[1][2];
    tmpBrick.pix[0][2] = activeBrick.pix[2][2];
    tmpBrick.pix[1][0] = activeBrick.pix[0][1];
    tmpBrick.pix[1][1] = activeBrick.pix[1][1];
    tmpBrick.pix[1][2] = activeBrick.pix[2][1];
    tmpBrick.pix[2][0] = activeBrick.pix[0][0];
    tmpBrick.pix[2][1] = activeBrick.pix[1][0];
    tmpBrick.pix[2][2] = activeBrick.pix[2][0];
    //Keep other parts of temporary block clear
    tmpBrick.pix[0][3] = 0;
    tmpBrick.pix[1][3] = 0;
    tmpBrick.pix[2][3] = 0;
    tmpBrick.pix[3][3] = 0;
    tmpBrick.pix[3][2] = 0;
    tmpBrick.pix[3][1] = 0;
    tmpBrick.pix[3][0] = 0;
    
  } else if (activeBrick.siz == 4){
    //Perform rotation around center "cross"
    tmpBrick.pix[0][0] = activeBrick.pix[0][3];
    tmpBrick.pix[0][1] = activeBrick.pix[1][3];
    tmpBrick.pix[0][2] = activeBrick.pix[2][3];
    tmpBrick.pix[0][3] = activeBrick.pix[3][3];
    tmpBrick.pix[1][0] = activeBrick.pix[0][2];
    tmpBrick.pix[1][1] = activeBrick.pix[1][2];
    tmpBrick.pix[1][2] = activeBrick.pix[2][2];
    tmpBrick.pix[1][3] = activeBrick.pix[3][2];
    tmpBrick.pix[2][0] = activeBrick.pix[0][1];
    tmpBrick.pix[2][1] = activeBrick.pix[1][1];
    tmpBrick.pix[2][2] = activeBrick.pix[2][1];
    tmpBrick.pix[2][3] = activeBrick.pix[3][1];
    tmpBrick.pix[3][0] = activeBrick.pix[0][0];
    tmpBrick.pix[3][1] = activeBrick.pix[1][0];
    tmpBrick.pix[3][2] = activeBrick.pix[2][0];
    tmpBrick.pix[3][3] = activeBrick.pix[3][0];
  } else {
    Serial.println("Brick size error");
  }
  
  //Now validate by checking collision.
  //Collision possibilities:
  //      -Brick now sticks outside field
  //      -Brick now sticks inside fixed bricks of field
  //In case of collision, we just discard the rotated temporary brick
  if ((!checkSidesCollision(&tmpBrick)) && (!checkFieldCollision(&tmpBrick))){
    //Copy temporary brick pix array to active pix array
    for (y=0;y<MAX_BRICK_SIZE;y++){
      for (x=0;x<MAX_BRICK_SIZE;x++){
        activeBrick.pix[x][y] = tmpBrick.pix[x][y];
      }
    }
  }
}

//Shift brick left/right/down by one if possible
void shiftActiveBrick(int dir){
  //Change position of active brick (no copy to temporary needed)
  if (dir == DIR_LEFT){
    activeBrick.xpos--;
  } else if (dir == DIR_RIGHT){
    activeBrick.xpos++;
  } else if (dir == DIR_DOWN){
    activeBrick.ypos++;
  }
  
  //Check position of active brick
  //Two possibilities when collision is detected:
  //    -Direction was LEFT/RIGHT, just revert position back
  //    -Direction was DOWN, revert position and fix block to field on collision
  //When no collision, keep activeBrick coordinates
  if ((checkSidesCollision(&activeBrick)) || (checkFieldCollision(&activeBrick))){
    //Serial.println("coll");
    if (dir == DIR_LEFT){
      activeBrick.xpos++;
    } else if (dir == DIR_RIGHT){
      activeBrick.xpos--;
    } else if (dir == DIR_DOWN){
      activeBrick.ypos--;//Go back up one
      addActiveBrickToField();
      activeBrick.enabled = false;//Disable brick, it is no longer moving
    }
  }
}

//Copy active pixels to field, including color
void addActiveBrickToField(){
  uint8_t bx,by;
  uint8_t fx,fy;
  for (by=0;by<MAX_BRICK_SIZE;by++){
    for (bx=0;bx<MAX_BRICK_SIZE;bx++){
      fx = activeBrick.xpos + bx;
      fy = activeBrick.ypos + by;
      
      if (fx>=0 && fy>=0 && fx<FIELD_WIDTH && fy<FIELD_HEIGHT && activeBrick.pix[bx][by]){//Check if inside playing field
        //field.pix[fx][fy] = field.pix[fx][fy] || activeBrick.pix[bx][by];
        field.pix[fx][fy] = activeBrick.pix[bx][by];
        field.color[fx][fy] = activeBrick.color;
      }
    }
  }
}

//Move all pix from te field above startRow down by one. startRow is overwritten
void moveFieldDownOne(uint8_t startRow){
  if (startRow == 0){//Topmost row has nothing on top to move...
    return;
  }
  uint8_t x,y;
  for (y=startRow-1; y>0; y--){
    for (x=0;x<FIELD_WIDTH; x++){
      field.pix[x][y+1] = field.pix[x][y];
      field.color[x][y+1] = field.color[x][y];
    }
  }
}

void checkFullLines(){
  int x,y;
  int minY = 0;
  for (y=(FIELD_HEIGHT-1); y>=minY; y--){
    uint8_t rowSum = 0;
    for (x=0; x<FIELD_WIDTH; x++){
      rowSum = rowSum + (field.pix[x][y]);
    }
    if (rowSum>=FIELD_WIDTH){
      //Found full row, animate its removal
      for (x=0;x<FIELD_WIDTH; x++){
        field.pix[x][y] = 0;
        printField();
        delay(100);
      }
      //Move all upper rows down by one
      moveFieldDownOne(y);
      y++; minY++;
      printField();
      delay(100);
      
      nbRowsThisLevel++; nbRowsTotal++;
      if (nbRowsThisLevel >= LEVELUP){
        nbRowsThisLevel = 0;
        brickSpeed = brickSpeed - SPEED_STEP;
        if (brickSpeed<200){
          brickSpeed = 200;
        }
      }
    }
  }
}

void clearField(){
  uint8_t x,y;
  for (y=0;y<FIELD_HEIGHT;y++){
    for (x=0;x<FIELD_WIDTH;x++){
      field.pix[x][y] = 0;
      field.color[x][y] = 0;
    }
  }
  for (x=0;x<FIELD_WIDTH;x++){//This last row is invisible to the player and only used for the collision detection routine
    field.pix[x][FIELD_HEIGHT] = 1;
  }
}

void scrollTextBlocked(char* text, int textLength, int color){
  unsigned long prevUpdateTime = 0;
  Serial.println(-textLength);
  
  for (int x=FIELD_WIDTH; x>-(textLength*8); x--){
    printText(text, textLength, x, 2, color);
    //Read buttons
    unsigned long curTime;
    do{
      readInput();
      curTime = millis();
    } while (((curTime - prevUpdateTime) < TEXTSPEED) && (curControl == BTN_NONE));//Once enough time  has passed, proceed
    
    prevUpdateTime = curTime;
  }
}



