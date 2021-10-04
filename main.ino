// ================================================================================
// Arduino Conway's Game of Life with MAX7219 with 8x8 Dot Matrix Display by OggyP
// ================================================================================
// Pins for MAX7219
const int DataOUT = 2;
const int CLK = 3;
const int Load = 4;

const int mainBtnPin = 5;   // Enabled / Disables regenerating the board every 'gensAmt' generations OR when in editing mode will draw on the selected position
const int potPin = 0;       // Constrols the speed of the simulation (higher the reading the longer the delay between frames)
const int pauseBtn = 6;     // Causes the game to pause and allows editing the current position
const int rowBtn = 7;       // Moves the cursor along the row
const int colBtn = 8;       // Moves the cursor along the column

bool grid[8][8];
bool newGrid[8][8];
byte byteArray[8];          // To be sent to MAX7219
int cursorShow[2] = {0, 0}; // Cursor position

// Handle flash of cursor
bool flash = true;
bool showFlash = true;

const int gensAmt = 150;     // How many generations before regenerating the board

int gens = 0;
bool paused = false;
bool doClear = true;


void setup()
{
  // initialize I/O pins.
  pinMode(mainBtnPin, INPUT_PULLUP);
  pinMode(pauseBtn, INPUT_PULLUP);
  pinMode(rowBtn, INPUT_PULLUP);
  pinMode(colBtn, INPUT_PULLUP);
  pinMode(DataOUT, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(Load, OUTPUT);
  digitalWrite(Load, HIGH);
  digitalWrite(CLK, HIGH); 
  //***************************MAX7219 Init*****************************************
  delay(100);
  SendDataMax7219(0xfc, 0xff); //Normal Operation 
  //delay(500);
  SendDataMax7219(0xfa, 0xf1); //Intensity
  //delay(500);
  //SendDataMax7219(0xff,0xff);//Display Test
  SendDataMax7219(0xfB, 0xf7); //Scan-Limit Register Format Digits 0,1,2,3 are ON
  //delay(500);
  SendDataMax7219(0xf9, 0x00); //Decode Mode - Code B Font
  delay(500);
  //********************************************************************************
  randomSeed(analogRead(5));       // Randomise the board
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      grid[y][x] = random(0, 2);
    }
  }
  Serial.begin(9600);
}

void loop()
{
  if (!digitalRead(pauseBtn)) { // Be aware due to how INPUT_PULLUP works, when the pin is pulled low it has been pressed
    // ON PAUSE / UNPAUSE
    doClear = false;
    cursorShow[0] = 7;
    cursorShow[1] = 0;
    paused = !paused;
    if (paused) {
      SendButtonRegister(3); // Show pause icon
    } else {
      SendButtonRegister(4); // Show play icon
    }
    delay(500);
    SendMatrixArray();
    while (!digitalRead(pauseBtn)) {
      delay(1); // Prevent the button from being held down
    }
  }
  if (!digitalRead(mainBtnPin) && !paused) {
    SendButtonRegister(!doClear); // Send tick / cross to show is clearing has been enabled / disabled
    doClear = !doClear;
    delay(500);
    while (!digitalRead(mainBtnPin)) {
      delay(1);
    }
  }

  if (paused) {
    if (!showFlash) {
      showFlash = true;
    }
    if (flash) {
      // Change cursor location on button click
      if (!digitalRead(rowBtn)) {
        if (cursorShow[0] == 7) {
          cursorShow[0] = 0;
        } else {
          cursorShow[0] ++;
        }
      }
      if (!digitalRead(colBtn)) {
        if (cursorShow[1] == 7) {
          cursorShow[1] = 0;
        } else {
          cursorShow[1] ++;
        }
      }

      // Draw to the grid where the cursor currently is
      if (!digitalRead(mainBtnPin)) {
        showFlash = false; // Prevent cursor being shown on the animation frame
        grid[cursorShow[1]][cursorShow[0]] = !grid[cursorShow[1]][cursorShow[0]];
        while (!digitalRead(mainBtnPin)) {
          delay(1);
        }
      }
    }
    // Set the grid to the byte array to be displayed
    for (int row = 0; row < 8; row++) {
      byteArray[row] = 0;
    }
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        // Do not draw the dot if the cursor is there
        if (y == cursorShow[1] && x == cursorShow[0]) {
          if (flash && showFlash) {
            bitSet(byteArray[y], x);
          }
        } else if (grid[y][x]) {
          bitSet(byteArray[y], x);
        }
      }
    }
    flash = !flash; // Alternate flash
    SendMatrixArray();
  } else {
    // Not paused
    // Set the grid to the byte array to be displayed
    for (int row = 0; row < 8; row++) {
      byteArray[row] = 0;
    }
    // Calculate the game
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        int neighbors = 0;
        for (int y1 = -1; y1 < 2; y1++) {
          for (int x1 = -1; x1 < 2; x1++) {
            if (x1 == 0 && y1 == 0) continue;
            int yToCheck = y1 + y;
            int xToCheck = x1 + x;

            // Handle looping of the game board
            if (yToCheck < 0) yToCheck = 8 - 1;
            if (yToCheck >= 8) yToCheck = 0;
            if (xToCheck < 0) xToCheck = 8 - 1;
            if (xToCheck >= 8) xToCheck = 0;

            if (grid[yToCheck][xToCheck]) {
              neighbors ++;
            }
          }
        }
        if ((neighbors == 3 || (grid[y][x] && neighbors == 2))) {
          newGrid[y][x] = true;
          bitSet(byteArray[y], x);
        } else {
          newGrid[y][x] = false;
        }
      }
    }
    // Copy buffer grid over to the current board
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        grid[y][x] = newGrid[y][x];
      }
    }
    gens++;
    SendMatrixArray();
  }

  if (doClear && (gens >= gensAmt || gens < 0)) { // Handle clearing of the board
    gens = 0;
    for (int y = 0; y < 8; y++) {
      for (int x = 0; x < 8; x++) {
        grid[y][x] = random(0, 2); // Randomise the board
      }
    }
  }
  delay(analogRead(potPin));
}

// Send byte array to the MAX chip
void SendMatrixArray() {
  SendDataMax7219(0xf1, byteArray[0]);
  SendDataMax7219(0xf2, byteArray[1]);
  SendDataMax7219(0xf3, byteArray[2]);
  SendDataMax7219(0xf4, byteArray[3]);
  SendDataMax7219(0xf5, byteArray[4]);
  SendDataMax7219(0xf6, byteArray[5]);
  SendDataMax7219(0xf7, byteArray[6]);
  SendDataMax7219(0xf8, byteArray[7]);
}

// Send a static image to the display to show that their input has been registered
void SendButtonRegister(int type) {
  if (type == 1) {
    // tick
    SendDataMax7219(0xf1, 0);
    SendDataMax7219(0xf2, 2);
    SendDataMax7219(0xf3, 4);
    SendDataMax7219(0xf4, 4);
    SendDataMax7219(0xf5, 72);
    SendDataMax7219(0xf6, 40);
    SendDataMax7219(0xf7, 16);
    SendDataMax7219(0xf8, 0);
  } else if (type == 0) {
    // cross
    SendDataMax7219(0xf1, 0);
    SendDataMax7219(0xf2, 66);
    SendDataMax7219(0xf3, 36);
    SendDataMax7219(0xf4, 24);
    SendDataMax7219(0xf5, 24);
    SendDataMax7219(0xf6, 36);
    SendDataMax7219(0xf7, 66);
    SendDataMax7219(0xf8, 0);
  } else if (type == 3) {
    // pause
    SendDataMax7219(0xf1, 0);
    SendDataMax7219(0xf2, 102);
    SendDataMax7219(0xf3, 102);
    SendDataMax7219(0xf4, 102);
    SendDataMax7219(0xf5, 102);
    SendDataMax7219(0xf6, 102);
    SendDataMax7219(0xf7, 102);
    SendDataMax7219(0xf8, 0);
  } else if (type == 4) {
    // play
    SendDataMax7219(0xf1, 0);
    SendDataMax7219(0xf2, 16);
    SendDataMax7219(0xf3, 24);
    SendDataMax7219(0xf4, 28);
    SendDataMax7219(0xf5, 28);
    SendDataMax7219(0xf6, 24);
    SendDataMax7219(0xf7, 16);
    SendDataMax7219(0xf8, 0);
  }
}

void SendDataMax7219(int HighByte, int LowByte)
{
  //---------------Send High Byte--------------
  for (int i = 7; i > -1; i--)
  {
    if (bitRead(HighByte, i) == HIGH)
    {
      digitalWrite(DataOUT, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);
      //delay(1);
      digitalWrite(CLK, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);


    }
    if (bitRead(HighByte, i) == LOW)
    {
      digitalWrite(DataOUT, LOW);
      //delay(1);
      digitalWrite(CLK, LOW);
      //delay(1);
      digitalWrite(CLK, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);
    }

  }
  //------------------------------------------
  //---------------Send Low Byte--------------
  for (int i = 7; i > -1; i--)
  {
    if (bitRead(LowByte, i) == HIGH)
    {
      digitalWrite(DataOUT, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);
      //delay(1);
      digitalWrite(CLK, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);
    }
    if (bitRead(LowByte, i) == LOW)
    {
      digitalWrite(DataOUT, LOW);
      //delay(1);
      digitalWrite(CLK, LOW);
      //delay(1);
      digitalWrite(CLK, HIGH);
      //delay(1);
      digitalWrite(CLK, LOW);
    }

  }
  //----------------------------------------
  //--------Load 16 bit number in MAX7219---
  //delay(1);
  digitalWrite(Load, LOW);
  //delay(1);
  digitalWrite(Load, HIGH);
  //delay(1);
  digitalWrite(Load, LOW);

  //----------------------------------------
}
