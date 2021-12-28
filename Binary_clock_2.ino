// Adafruit_NeoMatrix example for single NeoPixel Shield.
// Scrolls 'Howdy' across the matrix in a portrait (vertical) orientation.


//I2C device found at address 0x1D  !
//I2C device found at address 0x68  !



#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "RTClib.h"

#include <Wire.h>
#include <SparkFunADXL313.h> //Click here to get the library: http://librarymanager/All#SparkFun_ADXL313
ADXL313 adxl;


#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 5
#define PR A1
#define BUTT_PIN 12
#define MED_LIGHT 550
#define HIGH_LIGHT 340

// Declare a matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);
RTC_DS1307 rtc;

int32_t PRReading = 0;
int32_t PRReadingTemp = 0;
bool realtor[8][8];
bool gravityMode = false;
uint8_t DOT_NUM = 0;
size_t count = 0;
int8_t mode = 0;
int8_t modeLimit = 2;


class BitDot {
  
  public:

    // This sets the internal fixed location of the dot on the actual clock. That way it knows where to return after it's done bouncing around
    void setFixedLocation(int8_t setX, int8_t setY) {
      fixedX = setX;
      fixedY = setY;
      x = fixedX;
      y = fixedY;
    }

    // This takes an x and y reading off of the giro along with all the other x y locations of the other dots.
    void moveDot(int16_t xRead, int16_t yRead) {
      updatePulls(xRead, yRead); // Update the dots pull values
      int8_t tempx = shiftDot(pullx, x); // Creates a theoretical dot in the direction it wants to move. This accounts for the board but not other dots.
      int8_t tempy = shiftDot(pully, y);
      if (!realtor[tempx][tempy]) { // This where the dot wants to move against the locations of all the other dots.
        realtor[x][y] = false;  // this dot is no longer at that location
        x = tempx; // If that location was okay move the dot there.
        y = tempy;
        realtor[x][y] = true; // This dot is now at this location
      }
    }

    bool isInClockSpot() { // Where is the dang clock anyway?
      return x == fixedX && y == fixedY;
    }

    void setToClockSpot() { // Only need to use this once in setup to create the clock.
      x = fixedX;
      y = fixedY;
    }

    void setColor(uint32_t newCol0, uint32_t newCol1) {
      color0 = newCol0;
      color1 = newCol1;
      getRGBValues();
    }


    void displayDot() {
      if (fading) {
        fadeDot();
      } else {
        if (zero) {
          matrix.drawPixel(x, y, matrix.Color(red0, green0, blue0));
        } else {
          matrix.drawPixel(x, y, matrix.Color(red1, green1, blue1));
        }
      }
    }

    void setZeroOrOne(bool pos) {
      if (zero != pos) {
        fading = true;
        if (zero) {
          tempRed = red0;
          tempGreen = green0;
          tempBlue = blue0;
        } else {
          tempRed = red1;
          tempGreen = green1;
          tempBlue = blue1;
        }
      }
    }

  private:
    int8_t boundCheck(int8_t temp, int8_t incr) { // Make sure the new location is on the board.
      temp += incr;
      if (temp < 0) {
        return 0;
      } else if (7 < temp) {
        return 7;
      }
      return temp;
    }

    int8_t shiftDot(int32_t &tempPull, int8_t temp) { // Measure a pull value and see if it is enough to move a dot in that direction
      if (tempPull < -512) {
        tempPull = 0;
        return boundCheck(temp, -1);
      } else if (515 < tempPull) {
        tempPull = 0;
        return boundCheck(temp, 1);
      }
      return temp;
    }

    void updatePulls(int16_t xRead, int16_t yRead) {
      pullx += (xRead) / 4; // Increment how hard the dot is being pulled by the readings
      pully += (yRead)/ 4;
    }

    void fadeDot() {
      int8_t rate = 100;
      if (zero) {
        tempRed -= (red0 - red1) / rate;
        tempGreen -= (green0 - green1) / rate;
        tempBlue -= (blue0 - blue1) / rate;
        matrix.drawPixel(x, y, matrix.Color(tempRed, tempGreen, tempBlue));
        if (colorCheck(matrix.Color(tempRed, tempGreen, tempBlue), matrix.Color(red1, green1, blue1))) {
          fading = false;
          zero = false;
        }
      } else {
        tempRed -= (red1 - red0) / rate;
        tempGreen -= (green1 - green0) / rate;
        tempBlue -= (blue1 - blue0) / rate;
        matrix.drawPixel(x, y, matrix.Color(tempRed, tempGreen, tempBlue));
        if (colorCheck(matrix.Color(tempRed, tempGreen, tempBlue), matrix.Color(red0, green0, blue0))) {
          fading = false;
          zero = true;
        }
      }
    }

    bool colorCheck(uint16_t one, uint16_t two) {
      int16_t temp = one - 2;
      if (temp < 0) {
        temp = temp * -1;
      }
      return temp < 100;
    }

    void getRGBValues() {
      red0 = splitColor(color0, 'r');
      green0 = splitColor(color0, 'g');
      blue0 = splitColor(color0, 'b');
      red1 = splitColor(color1, 'r');
      green1 = splitColor(color1, 'g');
      blue1 = splitColor(color1, 'b');

    }


  uint8_t splitColor ( uint32_t c, char value ) {
     switch ( value ) {
      case 'r': return uint8_t(c >> 16);
      case 'g': return uint8_t(c >>  8);
      case 'b': return uint8_t(c >>  0);
      default:  return 0;
    }
  }
  
    int32_t pullx;
    int32_t pully;
    int8_t fixedX;
    int8_t fixedY;
    uint32_t color0;
    uint32_t color1;

    uint8_t red0;
    uint8_t green0;
    uint8_t blue0;

    uint8_t red1;
    uint8_t green1;
    uint8_t blue1;
    
    uint8_t tempRed;
    uint8_t tempGreen;
    uint8_t tempBlue;

    int8_t x;
    int8_t y;
    int32_t fadeColor;
    bool zero = true;
    bool fading = false;
  
};

// Create our array of 20 bits for this clock
BitDot BitDots[24];

void setup() {
  rtc.begin();
  Wire.begin();
  adxl.begin();
  matrix.begin();
  matrix.setBrightness(255);
  pinMode(PIN, OUTPUT);
  pinMode(BUTT_PIN, INPUT_PULLUP);
  pinMode(PR, INPUT);
  Serial.begin(115200);
  adxl.measureModeOn();

  // Set all the values on the matrix to open. DO THIS BEFORE YOU BUILD THE CLOCK
  emptyRealtor();

  // Sets the shape of the clock
  setClockMode();
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}


void loop() {
  matrix.clear();
  delay(1);
  if(adxl.dataReady()) {
    adxl.readAccel(); // read all 3 axis, they are stored in class variables: myAdxl.x, myAdxl.y and myAdxl.z
  }
  DateTime now = rtc.now(); // Get the current date
  PRReading = analogRead(PR);
  if (buttonCheck()) {
    setClockMode();
  }
  switch(mode) {
    case 0:
      setDotTime20Bit(now);
      break;
    case 1:
      setDotTime3Byte(now);
      break;
    case 2:
      setDotTime16Bit(now);
      break;
  }
  if (goGravityMode(adxl.x)) { // if the thing is getting tipped, lets party!!
    gravityMode = true;
    count = 1;
  } else {
    count++;
  }
  for (int i = 0; i < DOT_NUM; ++i) {
    if (gravityMode) {
      BitDots[i].moveDot(adxl.x, adxl.y); // move the dot!!
    }
    BitDots[i].displayDot();
  }
  if (!isAClock() && count == 500) { // if the dots are a mess and its time, reset the clock visual
    gravityMode = false;
    resetDots();
  }
  matrix.show();
}


/**
 * ====================================20 Bit Clock Logic====================================== 
 * 
 */

// This sets the fixed locations of all the dots accoring to the clock.
void buildClock20Bit() {
  int x = 0;
  int y = 0;
  for (int i = 0; i < DOT_NUM; ++i) {
    if (i < 4) {
      x = 2 + i;
      y = 6;
    } else if (i < 12 && 4 <= i) {
      x = i - 4;
      y = 3;
    } else if (12 <= i) {
      x = i - 12;
      y = 1;
    }
    BitDots[i].setFixedLocation(x, y);
    uint32_t color0 = buildColor(25, 25, 25);
    uint32_t color1 = buildColor(50, 25, 0);
    BitDots[i].setColor(color0, color1);
    realtor[x][y] = true;
  }
}

void setDotTime20Bit(DateTime now) { // This desides what color to display the dot. 
// set the bits for the hours
  int8_t temp = now.hour();
  if (temp > 12) {
    temp -= 12;
  }
  for (int i = 3; i >= 0; --i) {
    int32_t powOfTwo = 0.5 + pow(2, i);
    if (temp - powOfTwo >=0 && temp !=  0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }
  
// Set the bits for the minutes
  temp = now.minute();
  for (int i = 11; i >= 4; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 4));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }

// set the bits for the seconds
  temp = now.second();
  for (int i = 19; i >= 12; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 12));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }
}


/**
 * ====================================Byte Clock Logic====================================== 
 * 
 */
void buildClock3Byte() {
  uint8_t x;
  uint8_t y;
  uint16_t color = 0;
  for (int i = 0; i < DOT_NUM; ++i) {
    if (i < 8) {
      if (i < 4) {
        y = 7 - i;
        x = 7;
      } else {
        y = 11 - i;
        x = 6;
      }
    } else if (i < 16 && 8 <= i) {
      if (i < 12) {
        x = 4;
        y = 13 - i;
      } else {
        x = 3;
        y = 17 - i;
      }
    } else if (16 <= i) {
      if (i < 20) {
        x = 1;
        y = 19 - i;
      } else {
        x = 0;
        y = 23 - i;
      }
    }
    BitDots[i].setFixedLocation(x, y);
    BitDots[i].setColor(getColor3Byte(true, i), getColor3Byte(false, i));
    realtor[x][y] = true;
  }
}

void setDotTime3Byte(DateTime now) { // This desides what color to display the dot. 
// set the bits for the hours
  int8_t temp1st = floor(now.hour() / 10);
  int8_t temp2nd = now.hour() % 10;
  setDigitToByte(temp1st, 0);
  setDigitToByte(temp2nd, 4);
// Set the bits for the minutes
  temp1st = floor(now.minute() / 10);
  temp2nd = now.minute() % 10;
  setDigitToByte(temp1st, 8);
  setDigitToByte(temp2nd, 12);
// set the bits for the seconds
  temp1st = floor(now.second() / 10);
  temp2nd = now.second() % 10;
  setDigitToByte(temp1st, 16);
  setDigitToByte(temp2nd, 20);
}

// Helper function that takes in the digit you want to make into a byte and how far in the bitdot array you want to store this byte.
void setDigitToByte(int8_t digit, int8_t dotIndex) {
  for (int i = 0; i < 4; ++i) {
    int32_t powOfTwo = 0.5 + pow(2, 3 - i);
    int8_t check = digit - powOfTwo;
    int8_t adjustedIndex = i + dotIndex;
    if (check >= 0) {
      BitDots[adjustedIndex].setZeroOrOne(false);
      digit = check;
    } else {
      BitDots[adjustedIndex].setZeroOrOne(true);
    }
  }
}

uint16_t getColor3Byte(bool zeroOrOne, int8_t index) {
  uint8_t brightness = 55;
  if (index < 8) {
    return zeroOrOne ? matrix.Color(brightness / 2, 0, 0) : matrix.Color(brightness, brightness / 3, 0);
  } else if (index < 16 && 8 <= index) {
    return zeroOrOne ? matrix.Color(0, brightness / 2, 0) : matrix.Color(brightness / 2, brightness, brightness);
  } else if (16 <= index) {
    return zeroOrOne ? matrix.Color(0, 0, brightness) : matrix.Color(0, brightness  / 2, brightness);
  }
}

/**
 * ====================================16Bit Clock Logic====================================== 
 * 
 */

// This sets the fixed locations of all the dots accoring to the clock.
void buildClock16Bit() {
  int x = 0;
  int y = 0;
  for (int i = 0; i < DOT_NUM; ++i) {
    if (i < 4) {
      x = 2 + i;
      y = 6;
    } else if (i < 10 && 4 <= i) {
      x = i - 3;
      y = 3;
    } else if (10 <= i) {
      x = i - 9;
      y = 1;
    }
    BitDots[i].setFixedLocation(x, y);
    BitDots[i].setColor(getColor16Bit(true, i), getColor16Bit(false, i));
    realtor[x][y] = true;
  }
}

void setDotTime16Bit(DateTime now) { // This desides what color to display the dot. 
// set the bits for the hours
  int8_t temp = now.hour();
  if (temp > 12) {
    temp -= 12;
  }
  for (int i = 3; i >= 0; --i) {
    int32_t powOfTwo = 0.5 + pow(2, i);
    if (temp - powOfTwo >=0 && temp !=  0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }

// Set the bits for the minutes
  temp = now.minute();
  for (int i = 9; i >= 4; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 4));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }
  
// set the bits for the seconds
  temp = now.second();
  for (int i = 15; i >= 10; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 10));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setZeroOrOne(false);
    } else {
      BitDots[i].setZeroOrOne(true);
    }
  }
}

uint16_t getColor16Bit(bool zeroOrOne, int8_t index) {
  uint8_t brightness = 55;
  if (index < 4) {
    return zeroOrOne ? matrix.Color(brightness / 2, 0, 0) : matrix.Color(brightness, brightness / 3, 0);
  } else if (index < 10 && 4 <= index) {
    return zeroOrOne ? matrix.Color(0, brightness / 2, 0) : matrix.Color(brightness / 2, brightness, brightness);
  } else if (10 <= index) {
    return zeroOrOne ? matrix.Color(0, 0, brightness) : matrix.Color(0, brightness  / 2, brightness);
  }
}

bool goGravityMode(int16_t xval) { // 200 is all it takes to go into gravity mode! This of course senses if the x reading indicates the clock being tipped
  return xval > 200 || -200 > xval;
}


bool isAClock() { // Checks to see if the dots are in clockformation
  for (int i = 0; i < DOT_NUM; ++i) {
    if (!BitDots[i].isInClockSpot()) {
      return false;
    }
  }
  return true;
}


void resetDots() { // Goes through all the dots and resets them to clock formation.
  emptyRealtor();
  for (int i = 0; i < DOT_NUM; ++i) {
    BitDots[i].setToClockSpot();
  }
}

// Go through the realtor and wipe all memory of dot locations.
void emptyRealtor() {
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      realtor[j][i] = false;
    }
  }
}

// Check to see if the button has been pressed. Return true if the mode has changed. 
bool buttonCheck() {
  uint32_t buttonTimer = 0;
  uint32_t halfSec = 500;
  if (digitalRead(BUTT_PIN) == LOW) {
    while(digitalRead(BUTT_PIN) == LOW) {
      buttonTimer += 1;
      delay(1);
    }
    if (50 < buttonTimer && buttonTimer < halfSec) { mode += 1; }
    if (halfSec < buttonTimer) { mode -= 1; }
    if (mode > modeLimit) { mode = 0; }
    if (0 > mode) { mode = modeLimit; }
    emptyRealtor();
    return true; // Yes the mode has been changed.
  }
  return false; // No the mode has not been changed.
}

void setClockMode() {
  switch(mode) {
    case 0:
      DOT_NUM = 20;
      buildClock20Bit();
      break;
    case 1:
      DOT_NUM = 24;
      buildClock3Byte();
      break;
    case 2:
      DOT_NUM = 16;
      buildClock16Bit();
      break;
  }
}

uint32_t buildColor(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t red   = (uint8_t)(r << 16);
  uint16_t green = (uint8_t)(g <<  8);
  uint8_t  blue  = (uint8_t)(b <<  0);
  return; red+green+blue;
}