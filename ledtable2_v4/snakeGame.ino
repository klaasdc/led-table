/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Snake game
 */

void snakeInit(){
  //Snake start position and direction & initialise variables
  curLength = 3;
  xs[0]=3; xs[1]=2; xs[2]=1;
  ys[0]=FIELD_HEIGHT/2; ys[1]=FIELD_HEIGHT/2; ys[2]=FIELD_HEIGHT/2;
  dir = DIR_RIGHT;
  score = 0;
  //Generate random apple position
  ax = random(FIELD_WIDTH-1);
  ay = random(FIELD_HEIGHT-1);
  
  snakeGameOver = false;
}

void runSnake(){
  snakeInit();
  unsigned long prevUpdateTime = 0;
  boolean snakeRunning = true;
  while(snakeRunning){    
    //Check self-collision with snake
    int i=curLength-1;
    while (i>=2){
      if (collide(xs[0], xs[i], ys[0], ys[i], SNAKEWIDTH, SNAKEWIDTH, SNAKEWIDTH, SNAKEWIDTH)){
        Serial.println("Selfcollision");
        die();
      }
      i = i-1;
    }
    
    if (snakeGameOver){
      snakeRunning = false;
      break;
    }

    //Check collision of snake head with apple
    if (collide(xs[0], ax, ys[0], ay, SNAKEWIDTH, SNAKEWIDTH, SNAKEWIDTH, SNAKEWIDTH)){
      //Increase score and snake length;
      score = score +1;
      curLength = curLength +1;
      //Add snake segment with temporary position of new segments
      xs[curLength-1] = 255;
      ys[curLength-1] = 255;
      //Generate new apple position
      ax = random(FIELD_WIDTH-1);
      ay = random(FIELD_HEIGHT-1);
      //Generate apple color
      acolor = colorLib[random(6)];
    }
    
    //Shift snake position array by one
    i = curLength-1;
    while (i>=1){
      xs[i] = xs[i-1];
      ys[i] = ys[i-1];
      i = i-1;
    }
    //Determine new position of head of snake
    if (dir == DIR_RIGHT){
      xs[0] = xs[0] + SNAKEWIDTH;
    } 
    else if (dir == DIR_LEFT){
      xs[0] = xs[0] - SNAKEWIDTH;
    } 
    else if (dir == DIR_UP){
      ys[0] = ys[0] - SNAKEWIDTH;
    } 
    else {//DOWN
      ys[0] = ys[0] + SNAKEWIDTH;
    }
    
    //Check if outside playing field
    if ((xs[0]<0) || (xs[0]>=FIELD_WIDTH) || (ys[0]<0) || (ys[0]>=FIELD_HEIGHT)){
      die();
      break;
    }
    
    clearTablePixels();
    #ifdef USE_CONSOLE_OUTPUT
    clearTablePixelsConsole();
    #endif
   
    //Draw apple
    setTablePixel(ax,ay,acolor);

    //Draw snake
    for (int i=0; i<curLength; i++){
      setTablePixel(xs[i], ys[i], WHITE);
    }
    
    leds.show();
    #ifdef USE_CONSOLE_OUTPUT
    outputTableToConsole();
    #endif

    //Check buttons and set snake movement direction while we are waiting to draw the next move
    unsigned long curTime;
    boolean dirChanged = false;
    do{
      readInput();
      if (curControl == BTN_START){
        snakeRunning = false;
        break;
      }
      if (curControl != BTN_NONE && !dirChanged){//Can only change direction once per loop
        dirChanged = true;
        setDirection();
      }
      curTime = millis();
    } 
    while ((curTime - prevUpdateTime) <250);//Once enough time  has passed, proceed. The lower this number, the faster the game is
    prevUpdateTime = curTime;
  }
  
  fadeOut();
}

/* Set direction from current button state */
void setDirection(){
  switch(curControl){
    case BTN_LEFT:
      dir = DIR_LEFT;
      break;
    case BTN_RIGHT:
      dir = DIR_RIGHT;
      break;
    case BTN_DOWN:
      dir = DIR_DOWN;
      break;
    case BTN_UP:
      dir = DIR_UP;
      break;
    case BTN_START:
      break;
  }
}

/* Ending, show score */
void die(){
  snakeGameOver = true;
  Serial.println("die");
}

/* Collision detection function */
boolean collide(int x1, int x2, int y1, int y2, int w1, int w2, int h1, int h2){
  if ((x1+w1>x2) && (x1<x2+w2) && (y1+h1>y2) && (y1<y2+h2)){
    return true;
  } 
  else {
    return false;
  }
}

