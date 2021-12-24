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
#define MED_LIGHT 550
#define HIGH_LIGHT 340
#define DOT_NUM 20

// Declare a matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);
RTC_DS1307 rtc;

int32_t PRReading = 0;
int32_t PRReadingTemp = 0;
bool tempTimeBinary[20];
int8_t xArray[20];
int8_t yArray[20];
bool realtor[8][8];
bool gravityMode = false;
size_t count = 0;


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
    void moveDot(int16_t xRead, int16_t yRead, int8_t xArray[], int8_t yArray[]) {
      updatePulls(xRead, yRead); // Update the dots pull values
      int8_t tempx = shiftDot(pullx, x); // Creates a theoretical dot in the direction it wants to move. This accounts for the board but not other dots.
      int8_t tempy = shiftDot(pully, y);
      if (!realtor[tempx][tempy]) { // This where the dot wants to move against the locations of all the other dots.
        realtor[x][y] = false;
        x = tempx; // If that location was okay move the dot there.
        y = tempy;
        realtor[x][y] = true;
      }
    }

    bool isInClockSpot() { // Where is the dang clock anyway?
      return x == fixedX && y == fixedY;
    }

    void setToClockSpot() { // Only need to use this once in setup to create the clock.
      x = fixedX;
      y = fixedY;
    }
    
    int8_t getYVal() {
      return y;
    }

    int8_t getXVal() {
      return x;
    }
    
    void setColor(uint16_t newCol) {
      color = newCol;
    }

    void displayDot() {
      matrix.drawPixel(x, y, color);
    }

    void updatePulls(int16_t xRead, int16_t yRead) {
      //xMom += xRead / 2;
      //yMom +=yRead / 2;
      pullx += (xRead) / 4; // Increment how hard the dot is being pulled by the readings
      pully += (yRead)/ 4;
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
  
    bool stepCorrect(int8_t xArray[], int8_t yArray[], int8_t &tempx, int8_t &tempy) { // Make sure we aren't hitting any other dots
      for (int i = 0; i < DOT_NUM; ++i) {
        if (xArray[i] == tempx && yArray[i] == tempy) {
          return false;
        }
      }
      return true;
    }
    
    int32_t pullx;
    int32_t pully;
    int8_t fixedX;
    int8_t fixedY;
    int8_t xMom;
    int8_t yMom;
    uint16_t color;
    int8_t x;
    int8_t y;
  
};

// Create our array of 20 bits for this clock
BitDot BitDots[20];

void setup() {
  rtc.begin();
  Wire.begin();
  adxl.begin();
  matrix.begin();
  matrix.setBrightness(255);
  pinMode(PIN, OUTPUT);
  pinMode(PR, INPUT);
  Serial.begin(115200);
  adxl.measureModeOn();

  // Set all the values on the matrix to open. DO THIS BEFORE YOU BUILD THE CLOCK
  emptyRealtor();

  // Sets the shape of the clock
  buildClock();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}



void loop() {
  delay(1);
  if(adxl.dataReady()) {
    adxl.readAccel(); // read all 3 axis, they are stored in class variables: myAdxl.x, myAdxl.y and myAdxl.z
  }
  DateTime now = rtc.now(); // Get the current date
  PRReading = analogRead(PR);
  setDotTime(now);
  //updateXYArray();
  matrix.clear();
  if (!goGravityMode(adxl.x)) { // if the thing is getting tipped, lets party!!
    count++;
  } else {
    gravityMode = true;
    count = 1;
  }
  for (int i = 0; i < DOT_NUM; ++i) {
    if (gravityMode) {
      BitDots[i].moveDot(adxl.x, adxl.y, xArray, yArray); // move the dot!!
      //xArray[i] = BitDots[i].getXVal(); // update the array with all locations of all the dots
      //yArray[i] = BitDots[i].getYVal();
    }
    BitDots[i].displayDot();
  }
  if (!isAClock() && count == 500) { // if the dots are a mess and its time, reset the clock visual
    gravityMode = false;
    resetDots();
  }
  matrix.show();
}


// This sets the fixed locations of all the dots accoring to the clock.
void buildClock() {
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
    realtor[x][y] = true;
  }
}

void setDotTime(DateTime now) { // This desides what color to display the dot. 
// set the bits for the hours
  int8_t temp = now.hour();
  if (temp > 12) {
    temp -= 12;
  }
  for (int i = 3; i >= 0; --i) {
    int32_t powOfTwo = 0.5 + pow(2, i);
    if (temp - powOfTwo >=0 && temp !=  0) {
      temp -= powOfTwo;
      BitDots[i].setColor(getColor(true));
    } else {
      BitDots[i].setColor(getColor(false));
    }
  }
  
// Set the bits for the minutes
  temp = now.minute();
  for (int i = 11; i >= 4; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 4));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setColor(getColor(true));
    } else {
      BitDots[i].setColor(getColor(false));
    }
  }

// set the bits for the seconds
  temp = now.second();
  for (int i = 19; i >= 12; --i) {
    int32_t powOfTwo = 0.5 + pow(2, (i - 12));
    if (temp - powOfTwo >=0 && temp != 0) {
      temp -= powOfTwo;
      BitDots[i].setColor(getColor(true));
    } else {
      BitDots[i].setColor(getColor(false));
    }
  }
}

uint16_t getColor(bool oneOrZero) { // This looks at the photo sensor and returns different color themes depending on brightness
 //Serial.println(PRReading);
 if (PRReading < HIGH_LIGHT) { //Messing with the set brightness function is a major pain. So here the brightness is adjusted manually but just putting in smaller values.
    return oneOrZero ? matrix.Color(50, 20, 0) : matrix.Color(25, 25, 25);
  } else if (PRReading < MED_LIGHT && HIGH_LIGHT <= PRReading) {
    return oneOrZero ? matrix.Color(25, 8, 0) : matrix.Color(8, 4, 8);
  } else if (MED_LIGHT <= PRReading) {
    return oneOrZero ? matrix.Color(8, 0, 0) : matrix.Color(0, 4, 8);
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
  for (int i = 0; i < DOT_NUM; ++i) {
    BitDots[i].setToClockSpot();
  }
}

void emptyRealtor() {
  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      realtor[j][i] = false;
    }
  }
}