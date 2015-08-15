#include "LedControlMS.h"
#define NBR_MTX 2

// This is specific to how I set up my 8x8 board
LedControl lc = LedControl(12, 11, 10, NBR_MTX);

// This is my hack to let me reset the program when I die
void(* resetFunc) (void) = 0;

boolean rButton = LOW;
boolean lButton = LOW;
boolean currentlyPressed = false;
boolean onePress = false;
boolean extend = false;
boolean dead = false;

// This initializes the snake to an array of 64, but as long as I keep track of snakeSize it won't matter
// It's the lazy way of getting around dynamic arrays in C
int snakeX[64] = {0};
int snakeY[64] = {0};
int snakeSize = 2;

// The left button uses 2 and the right uses 3
int buttonPin2 = 2;
int buttonPin3 = 3;
int turn = 1;
int fruitX = 0;
int fruitY = 0;
int snakeFirstX = 0;
int snakeFirstY = 0;

long previousMillis = 0;
long interval = 400;

char tbs[16];

void setup()
{
  // Getting the random seed for the beginning and fruit
  randomSeed(analogRead(0));

  setFruit();

  //The next few lines are for setting the 8x8 board up
  Serial.begin (9600);
  Serial.println("Setup");

  for (int i = 0; i < NBR_MTX; i++)
  {
    lc.shutdown(i, false);
    lc.setIntensity(i, 8);
    lc.clearDisplay(i);
  }

  getStart();
}

int getStart()
{
  // Basically this puts the snake in one of four positions in the middle of the board
  // Going in the direction it's facing
  // Could probably be cleaner
  
  turn = random(4) + 1;
  int addition = random(2);
  if (turn == 1 || turn == 3)
  {
    snakeY[0] = 3 + addition;
    snakeY[1] = 3 + addition;

    snakeX[0] = 3;
    snakeX[1] = 3;

    if (turn == 1)
      snakeX[0]++;
    else if (turn == 3)
      snakeX[1]++;
  }
  else if (turn == 2 || turn == 4)
  {
    snakeX[0] = 3 + addition;
    snakeX[1] = 3 + addition;

    snakeY[0] = 3;
    snakeY[1] = 3;

    if (turn == 2)
      snakeY[1]++;
    else if (turn == 4)
      snakeY[0]++;
  }
}

int getFruit()
{
  return random(8);
}

void setFruit()
{
  // This doesn't work and I'm not sure why
  // As in it still places fruits on the snake sometimes
  
  while (true)
  {  
    fruitX = getFruit();
    fruitY = getFruit();

    for (int i = 0; i < snakeSize; i++)
      if ((snakeX[i] == fruitX) && (snakeY[i] == fruitY))
        continue;

    return;
  }
}

boolean growSnake()
{
  // This simply increases the snake size variable
  // And then moves each value of the array over one
  // But keeps the first one the same, causing the grow to show up after one "turn"
  
  snakeSize++;

  for (int i = snakeSize - 1; i > 0; i--)
  {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] = snakeFirstX;
  snakeY[0] = snakeFirstY;

  if (snakeSize == 5)
    interval = 350;
  else if (snakeSize == 10)
    interval = 300;

  return false;
}

void Move()
{
  // 1 is left, 2 up, 3 right, and 4 down
  // This was a design choice for code simplicity
  
  if (turn == 1)
    snakeX[snakeSize - 1]++;
  else if (turn == 2)
    snakeY[snakeSize - 1]--;
  else if (turn == 3)
    snakeX[snakeSize - 1]--;
  else if (turn == 4)
    snakeY[snakeSize - 1]++;
}

void roundWalls()
{
  // Obsolete function, but could be used if you want the snake to not hit walls
  
  for (int i = 0; i < snakeSize; i++)
  {
    if (snakeX[i] >= 8)
      snakeX[i] = 0;
    if (snakeX[i] <= -1)
      snakeX[i] = 7;

    if (snakeY[i] >= 8)
      snakeY[i] = 0;
    if (snakeY[i] <= -1)
      snakeY[i] = 7;
  }
}

void checkFruit()
{
  // Checks if the snake ate a fruit and if so, gets a new one

  if ((snakeX[snakeSize - 1] == fruitX) && (snakeY[snakeSize - 1] == fruitY))
  {
    setFruit();
    lc.setLed(0, fruitX, fruitY, false);
    extend = true;
  }

  lc.setLed(0, fruitX, fruitY, true);
}

void checkButtons()
{
  // currentlyPressed makes it so you can't press the other button if it's currently pressed
  // onePress only allows you to press one button per cycle, without it you could do u-turns and crap between cycles

  if (lButton == LOW && !currentlyPressed && !onePress)
  {
    currentlyPressed = true;
    Serial.println("left pressed");
    Serial.println(turn);
    if (turn > 1)
      turn--;
    else
      turn = 4;
    onePress = true;
  }
  else if (rButton == LOW && !currentlyPressed && !onePress)
  {
    currentlyPressed = true;
    Serial.println("right pressed");
    Serial.println(turn);
    if (turn < 4)
      turn++;
    else
      turn = 1;
    onePress = true;
  }

  if (rButton == HIGH && lButton == HIGH && currentlyPressed)
  {
    currentlyPressed = false;
  }
}

void die()
{
  // Had to use the dead boolean because buttons still worked while dying
  // This basically just flashes it a few tims and then calls the resetFunc() which resets the game

  dead = true;
  for (int delayTime = 100; delayTime < 1000; delayTime = delayTime * 8)
  {
    for (int i = 0; i < 3; i++)
    {
      for (int j = 0; j < snakeSize; j++)
        lc.setLed(0, snakeX[j], snakeY[j], true);
      delay (delayTime);

      for (int j = 0; j < snakeSize; j++)
        lc.setLed(0, snakeX[j], snakeY[j], false);
      delay (delayTime);
    }
  }

  resetFunc();
}

boolean hitWall()
{
  // Determines if the snake ate itself or hit a wall

  for (int i = 0; i < snakeSize; i++)
  {
    if ((snakeX[i] < 0) || (snakeY[i] < 0) || (snakeX[i] > 7) || (snakeY[i] > 7))
      return true;

    sprintf(tbs, "X: %d Y: %d", snakeX[i], snakeY[i]);
    Serial.println(tbs);

    for (int j = 0; j < snakeSize; j++)
    {
      if ((i != j) && (snakeX[i] == snakeX[j]) && (snakeY[i] == snakeY[j]))
        return true;
    }
  }

  return false;
}

void loop()
{
  // At this point the loop should be pretty understandable 

  if (!dead)
  {
    unsigned long currentMillis = millis();

    rButton = digitalRead(buttonPin3);
    lButton = digitalRead(buttonPin2);

    // My solution to delay() not accepting inputs, this basically will space everything out similar to a delay
    // But because checkButtons() is outside of it, it still checks if the buttons are read
    
    if (currentMillis - previousMillis > interval)
    {
      onePress = false;
      previousMillis = currentMillis;

      snakeFirstX = snakeX[0];
      snakeFirstY = snakeY[0];
      lc.setLed(0, snakeFirstX, snakeFirstY, false);

      for (int i = 0; i < snakeSize - 1; i++)
      {
        snakeX[i] = snakeX[i + 1];
        snakeY[i] = snakeY[i + 1];
      }

      Move();
      if (hitWall())
        die();

      if (extend)
        extend = growSnake();

      checkFruit();

      for (int i = 0; i < snakeSize; i++)
        lc.setLed(0, snakeX[i], snakeY[i], true);
    }

    checkButtons();
  }
}
