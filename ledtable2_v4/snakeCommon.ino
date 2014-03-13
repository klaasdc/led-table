/* LedTable for Teensy3
 *
 * Written by: Klaas De Craemer, 2013
 * https://sites.google.com/site/klaasdc/led-table
 * 
 * Snake game common defines/variables
 */

/* ------ Game related ------ */
int curLength;//Curren length of snake
int xs[127];//Array containing all snake segments,
int ys[127];// max snake length is array length
int dir;//Current Direction of snake
int score;
#define SNAKEWIDTH  1 //Snake width

boolean snakeGameOver;

int ax = 0;//Apple x position
int ay = 0;//Apple y position
int acolor = RED;
