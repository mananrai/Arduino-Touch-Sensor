/*
 * ENGR 40M Lab 3c
 * 
 * @author Manan Rai
 *  - version 3 @ Nov 16, 17
 *      added scanAxes and radiate patterns
 *  - version 2 @ Nov 15, 17
 *      added raindrops pattern and fixed touch sensing bugs
 *  - version 1 @ Nov 9, 17
 *      made cube respond to touch sensing
 *  
 *  - referred to: InputControlledLEDCube.ino @author Manan Rai
 *                 raindrop_starter.ino (https://web.stanford.edu/class/engr40m/labs.html)
 * 
 * This is a code for the LED Cube that includes Capacitive Touch Sensing.
 * 
 * It can draw four different patterns on the cube: scanning the axes, raindrops,
 * a hollow sphere radiating, and a solid sphere radiating out from any LED.
 *
 */

// Defining pins for anodes (+), cathodes (-) and the touch sensor
const byte  ANODE_PINS[8] = {3, 5, 7, 9, 2, 4, 6, 8};
const byte  CATHODE_PINS[8] = {A4, A2, 11, 13, A5, A3, 10, 12};
const int   TOUCH_SENSOR_DRIVE = A1;
const int   TOUCH_SENSOR_SENSE = A0;

// Defining the time delay
const int   TIME_DELAY = 150;

// Defining the maximum l-squared-norm Distance of a 4x4x4 cube (longest distance between any two vertices)
const float CUBE_DIAGONAL = 5.2;

// Function prototypes
void display(byte ledPattern[4][4][4], byte numberOfCycles);
inline byte getLEDState(byte ledPattern[4][4][4], byte aNum, byte cNum);
void gamePlay(byte ledPattern[4][4][4], int k);
void rain(byte ledPattern[4][4][4]);
void chooseRandomInTopPlane(byte ledPattern[4][4][4]);
void movePatternDown(byte ledPattern[4][4][4]);
void scanAxes(byte ledPattern[4][4][4]);
void changeXPlane(int layer, byte val, byte ledPattern[4][4][4]);
void changeYPlane(int layer, byte val, byte ledPattern[4][4][4]);
void changeZPlane(int layer, byte val, byte ledPattern[4][4][4]);
void radiate(byte ledPattern[4][4][4], bool hollowSphere);
float findDistance(int x1, int y1, int z1, int x2, int y2, int z2);
void resetCube(byte ledPattern[4][4][4]);

/*
 * Function: setup
 * ----------------
 * This will run once to initialize the program.
 * 
 */
void setup()
{
  // Make all of the anode (+) and cathode (-) wire pins outputs
  // and set all LEDs off
  for (byte i = 0; i < 8; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    digitalWrite(ANODE_PINS[i], HIGH);  // Note that pin output for anodes is driven into
                                        // a pMOS, so a HIGH output drives LOW to the LED
    pinMode(CATHODE_PINS[i], OUTPUT);
    digitalWrite(CATHODE_PINS[i], HIGH);
  }

  // Set the Touch Sensor drive and sensing pins
  pinMode(TOUCH_SENSOR_DRIVE, OUTPUT);
  pinMode(TOUCH_SENSOR_SENSE, INPUT);

  // Initialize serial communication
  // (to be read by Serial Monitor on the computer)
  Serial.begin(115200);
  Serial.setTimeout(100);
}

/*
 * Function: loop
 * ----------------
 * This will run continuously after setup() finishes running. It reads the state of
 * the Capacitive Touch Sensor using analogRead. A value greater than 980 signifies
 * a capacitance indicative of touch, and it then draws a pattern on the cube.
 * 
 * It randomly chooses from 4 different patterns: plane-by-plane scanning, raindrops,
 * a hollow sphere radiating out, and a solid sphere radiating out.
 * 
 */
void loop()
{
  static byte ledPattern[4][4][4]; // 1 for on, 0 for off

  // Charge the capacitor
  digitalWrite(TOUCH_SENSOR_DRIVE, HIGH);
  
  if (analogRead(TOUCH_SENSOR_SENSE) >= 980) {
    int choice = random(1, 2);
    if (choice == 0) {
      scanAxes(ledPattern);
    } else if (choice == 1) {
      rain(ledPattern);
    } else {
      int hollowSphere = random(0, 2);
      radiate(ledPattern, (hollowSphere == 1));
    }
  }

  // Discharge the capacitor
  digitalWrite(TOUCH_SENSOR_DRIVE, LOW);
}

/* 
 * Function: display
 * -------------------
 * Runs through one multiplexing cycle of the LEDs, controlling which LEDs are on.
 *
 * During this function, LEDs that should be on will be turned on momentarily, one
 * row at a time. When this function returns, all the LEDs will be off again, so
 * it needs to be called continuously for LEDs to be on.
 * 
 */
void display(byte ledPattern[4][4][4])
{
  long currentTime = millis();
  // Executes for atleast TIME_DELAY milliseconds
  while (millis() < currentTime + TIME_DELAY) {
    // For each anode (+) wire
    for (byte aNum = 0; aNum < 8; aNum++) {
  
      // For each cathode (-) wire
      for (byte cNum = 0; cNum < 8; cNum++) {
        byte value = getLEDState(ledPattern, aNum, cNum);
  
        // If it should be on, drive the LED cathode LOW, so the LED turns on
        if (value > 0) {
          digitalWrite(CATHODE_PINS[cNum], LOW);
        // else drive it HIGH so the LED is off.
        } else {
          digitalWrite(CATHODE_PINS[cNum], HIGH);
        }
      }
  
      // Drive the LED anode HIGH
      digitalWrite(ANODE_PINS[aNum], LOW);
      delayMicroseconds(500);
      // Drive the LED anode LOW
      digitalWrite(ANODE_PINS[aNum], HIGH);
    }
  }
}

/*
 * Function: getLEDState
 * ----------------------
 * Returns the state of the LED in a 4x4x4 pattern array, each dimension
 * representing an axis of the LED cube, that corresponds to the given anode (+)
 * wire and cathode (-) wire number.
 *
 * This function is called by display(), in order to find whether an LED for a
 * particular anode (+) wire and cathode (-) wire should be switched on.
 * 
 */
inline byte getLEDState(byte ledPattern[4][4][4], byte aNum, byte cNum)
{
  /*
   * The x-coordinate in the cube changes every time the cathode changes range from (0-3) to (4-7).
   * The y-coordinate in the cube changes every time the anode   changes range from (0-3) to (4-7).
   * The z-coordinate in the cube is determined by both the cathode and anode number.
   *      If the cathode is in the range (0-3),
   *          and the anode is in the range (0-3), it is 0.
   *          else if anode is in the range (4-7), it is 3.
   *      If the cathode is in the range (4-7),
   *          and the anode is in the range (0-3), it is 1.
   *          else if anode is in the range (4-7), it is 2.
   * 
   */
  int x = cNum % 4;
  int y = aNum % 4;
  int z;
  if (cNum >= 4) {
    if (aNum < 4) {
      z = 1;
    } else {
      z = 2;
    }
  } else if (aNum < 4) {
    z = 0;
  } else {
    z = 3;
  }
  
  return ledPattern[x][y][z];
}

/**
 * Function: rain
 * ----------------
 * Mimics a raindrop pattern on the cube, using helper functions to move the drops down.
 * 
 */
void rain(byte ledPattern[4][4][4])
{
  int numOfDrops = 15;
  // Animate numOfDrops raindrops
  for (int i = 0; i < numOfDrops; i++) {
    chooseRandomInTopPlane(ledPattern);
    movePatternDown(ledPattern);
    display(ledPattern);
  }
}

/**
 * Function: chooseRandomInTopPlane
 * ----------------------------------
 * Chooses a random starting point for a new drop on the top-most plane.
 * Assumes that the top plane has already been cleared (all ligths set off).
 * 
 */
void chooseRandomInTopPlane(byte ledPattern[4][4][4]) {
  byte x = random(0, 4);
  byte y = random(0, 4);
  ledPattern[x][y][3] = 1;
  //display(ledPattern);
}

/**
 * Function: movePatternDown
 * ---------------------------
 * Moves the pattern currently on the cube one plane down. It turns the top plane off and
 * copies the pattern of every other plane on the plane one level down.
 * 
 */
void movePatternDown(byte ledPattern[4][4][4]) {
  for (byte z = 0; z < 4; z++) {
    for (byte y = 0; y < 4; y++) {
      for (byte x = 0; x < 4; x++) {
        if (z == 3) ledPattern[x][y][3] = 0; // If we're at the top plane, set every LED's state to 0
        else ledPattern[x][y][z] = ledPattern[x][y][z + 1]; // otherwise, copy the upper level's states.
      }
    }
  }
}

/**
 * Function: scanAxes
 * --------------------
 * Moves planes of light across all three axes one by one.
 * 
 */
void scanAxes(byte ledPattern[4][4][4]) {
  resetCube(ledPattern);

  // X-Axis
  for (int x = 3; x >= 0; x--) {
    // Set the yz-plane on
    changeXPlane(x, 1, ledPattern);
    /* 
     * If the x-coordinate is not the top-most (3)
     * then set the previous layer off and turn the
     * current layer on.
     */
    if (x < 3) { 
      changeXPlane(x + 1, 0, ledPattern); 
    }
    display(ledPattern);
  }
  
  // Move the plane backwards
  for (int x = 1; x <= 3; x++) {
    changeXPlane(x, 1, ledPattern);
    changeXPlane(x - 1, 0, ledPattern);
    display(ledPattern);
  }
  resetCube(ledPattern);
  
  // Y-Axis
  for (int y = 3; y >= 0; y--) {
    // Set the xz-plane on
    changeYPlane(y, 1, ledPattern);
    /*
     * If the y-coordinate is not the top-most (3)
     * then set the previous layer off and turn the
     * current layer on.
     */
    if (y < 3) { 
      changeYPlane(y + 1, 0, ledPattern); 
    }
    display(ledPattern);
  }
  
  // Move the plane backwards
  for (int y = 1; y <= 3; y++) {
    changeYPlane(y, 1, ledPattern);
    changeYPlane(y - 1, 0, ledPattern);
    display(ledPattern);
  }
  resetCube(ledPattern);

  // Z-Axis
  for (int z = 3; z >= 0; z--) {
    // Set the xy-plane on
    changeZPlane(z, 1, ledPattern);
    /*
     * If the z-coordinate is not the top-most (3)
     * then set the previous layer off and turn the
     * current layer on.
     */
    if (z < 3) { 
      changeZPlane(z + 1, 0, ledPattern); 
    }
    display(ledPattern);
  }
  
  // Move the plane backwards
  for (int z = 1; z <= 3; z++) {
    changeZPlane(z, 1, ledPattern);
    changeZPlane(z - 1, 0, ledPattern);
    display(ledPattern);
  }
}

/**
 * Function: changeXPlane
 * ------------------------
 * Changes the value of each LED on the YZ-Plane to the given 'val'
 * 
 */
void changeXPlane(int layer, byte val, byte ledPattern[4][4][4]) {
  for (int y = 0; y < 4; y++) {
    for (int z = 0; z < 4; z++) {
      ledPattern[layer][y][z] = val;
    }
  }
}

/**
 * Function: changeXPlane
 * ------------------------
 * Changes the value of each LED on the XZ-Plane to the given 'val'
 * 
 */
void changeYPlane(int layer, byte val, byte ledPattern[4][4][4]) {
  for (int x = 0; x < 4; x++) {
    for (int z = 0; z < 4; z++) {
      ledPattern[x][layer][z] = val;
    }
  }
}

/**
 * Function: changeXPlane
 * ------------------------
 * Changes the value of each LED on the XY-Plane to the given 'val'
 * 
 */
void changeZPlane(int layer, byte val, byte ledPattern[4][4][4]) {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      ledPattern[x][y][layer] = val;
    }
  }
}

/**
 * Function: radiate
 * -------------------
 * Makes light radiate from a chosen central LED. It randomly selects an LED to acts
 * as the centre, and then selects all LED's within a range of distances to trun on.
 * 
 */
void radiate(byte ledPattern[4][4][4], bool hollowSphere) {
  resetCube(ledPattern);

  // Choose a random LED to be the centre
  int x = random(0, 4);
  int y = random(0, 4);
  int z = random(0, 4);

  // Flash the chosen LED
  for (int i = 0; i < 5; i++) {
    ledPattern[x][y][z] = 1;
    display(ledPattern);
    ledPattern[x][y][z] = 0;
    display(ledPattern);    
  }

  // Radiate from the chosen centre LED
  float minDistance = 0;
  float maxDistance = 0.5;
  while (maxDistance <= CUBE_DIAGONAL) {
    
    // For all LED's within the distance range from the centre
    for (int x2 = 0; x2 < 4; x2++) {
      for (int y2 = 0; y2 < 4; y2++) {
        for (int z2 = 0; z2 < 4; z2++) {
          float distance = findDistance(x, y, z, x2, y2, z2);

          // Turn them all on
          if (minDistance <= distance && distance <= maxDistance) {
            ledPattern[x2][y2][z2] = 1;
          } else {
            ledPattern[x2][y2][z2] = 0;
          }
        }
      }
    }
    display(ledPattern);
    /*
     * In order to make a solid sphere radiate out, we increase the minimum distance as well
     */
    if (!hollowSphere) minDistance += 0.5;
    maxDistance += 0.5; // having a smaller increase in distance makes this work for longer
  }
}

/**
 * Function: findDistance
 * ------------------------
 * This returns the distance between 2 vertices of a cube,
 *     = {(x1 - x2) ^ 2 + (y1 - y2) ^ 2 + (z1 - z2) ^ 2} ^ (-1/2)
 * 
 */
float findDistance(int x1, int y1, int z1, int x2, int y2, int z2) {
  return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2));
}

/**
 * Function: resetCube
 * ---------------------
 * Sets the states of all the LED's in the cube to OFF.
 * 
 */
void resetCube(byte ledPattern[4][4][4]) {
  for (int z = 0; z < 4; z++) {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        ledPattern[x][y][z] = 0;
      }
    }
  }
}

