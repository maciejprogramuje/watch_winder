#include <TM1637Display.h>
#include <Stepper.h>
#define STEPS 2048 // the number of steps in one revolution of your motor (28BYJ-48) 
#define BUTTON 2
#define DISP_CLK 6
#define DISP_DIO 5
// **************************************************************************
// ******* VARIABLE DEFINITIONS *********************************************
// **************************************************************************
const float revBig = 1.5; // revolutions factor for one case revolution
const float oneRevTime = 9.53; // time of one case revolution
const int revCont = 150; // number of revolutions in continous mode
const int pauseMinutes = 36; // number of minutes pause after 100 revolutions
int revNumbers[] = {750, 1500, 2000, 2500}; // revolutions number to full tension
// **************************************************************************
// ******** DISPLAY DEFINITIONS *********************************************
// **************************************************************************
uint8_t dispCont[] = {SEG_A | SEG_F | SEG_E | SEG_D, SEG_G | SEG_E | SEG_D | SEG_C, SEG_E | SEG_G | SEG_C, SEG_F | SEG_G | SEG_E | SEG_D}; // Cont
uint8_t dispC[] = {SEG_A | SEG_F | SEG_E | SEG_D, 0, 0, 0}; // C
uint8_t dispFull[] = {SEG_F | SEG_A | SEG_G | SEG_E, SEG_E | SEG_D | SEG_C, SEG_B | SEG_C, SEG_B | SEG_C}; // Full
uint8_t dispF[] = {SEG_F | SEG_A | SEG_G | SEG_E, 0, 0, 0}; // F
uint8_t dispDirectionMode_0[] = {SEG_G | SEG_B | SEG_C, SEG_F | SEG_E | SEG_G, 0, 0}; // -| -| |- |-
uint8_t dispDirectionMode_1[] = {SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, 0, 0}; // |- |- |- |-
uint8_t dispDirectionMode_2[] = {SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, 0, 0}; // -| -| -| -|
uint8_t dispDone[] = {SEG_B | SEG_C | SEG_D | SEG_E | SEG_G, SEG_G | SEG_C | SEG_D | SEG_E, SEG_C | SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G}; // donE
uint8_t dispCw[] = {SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G}; // |- |-
uint8_t dispCCw[] = {SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C}; // -| -|
uint8_t dispBothSides[] = {SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G}; // -| |-
uint8_t dispHelo[] = {SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G}; // HELO
// **************************************************************************
Stepper stepper(STEPS, 8, 10, 9, 11);
TM1637Display disp(DISP_CLK, DISP_DIO);
int menuStep = 0;
int contOrFullMode = 0;
int tempRevCont;
int revFull;
int tempRevFull;
int tempPauseMinutes;
unsigned long tempPauseSeconds;
int revNumberMode = -1;
int revDirection = 1;             // 0 both sides, 1 clock-wise, 2 counter clock-wise
int revDirectionMode = -1;    // 0 both sides, 1 clock-wise, 2 counter clock-wise
// **************************************************************************
void setup() {
  Serial.begin(9600);
  stepper.setSpeed(10);
  disp.setBrightness(0x0A);
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), onPushButton, FALLING);
  setTempValuables();

  menuStep = 3;
  contOrFullMode = 0;
  revDirectionMode = 0;
}
// **************************************************************************
void loop() {
  delay(500);

  if (menuStep == 0) {
    setContOrFullMode();
  } else if (menuStep == 1) {
    setDirection();
  } else if (menuStep == 2) {
    if (contOrFullMode == 0) {
      menuStep = 3;
      doContinousTension();
    } else {
      setRevolutionNumber();
    }
  } else if (menuStep == 3) {
    if (contOrFullMode == 0) {
      doContinousTension();
    } else if (contOrFullMode == 1) {
      doFullTension();
    }
  }

  Serial.println("loop");
}
// **************************************************************************
void onPushButton() {
  static unsigned long lastTime;
  unsigned long timeNow = millis();
  if (timeNow - lastTime < 500)
    return;
  menuStep++;
  if (menuStep > 3) {
    menuStep = 0;
    contOrFullMode = 0;
    revDirectionMode = -1;
    revNumberMode = -1;
  }
  lastTime = timeNow;

  loop();
}
// **************************************************************************
void setContOrFullMode() {
  contOrFullMode++;
  if (contOrFullMode > 1) {
    contOrFullMode = 0;
  }

  if (contOrFullMode == 0) {
    disp.setSegments(dispCont, 4, 0);
  } else if (contOrFullMode == 1) {
    disp.setSegments(dispFull, 4, 0);
  }
  delay(2000);
}
// **************************************************************************
void doContinousTension() {
  if (tempRevCont > 0) {
    if (tempRevCont % 2 == 1) {
      disp.setSegments(dispC, 2, 0);
      if (revDirectionMode == 0) {
        disp.setSegments(dispDirectionMode_0, 2, 2);
      } else if (revDirectionMode == 1) {
        disp.setSegments(dispDirectionMode_1, 2, 2);
      } else if (revDirectionMode == 2) {
        disp.setSegments(dispDirectionMode_2, 2, 2);
      }
    } else {
      disp.showNumberDec(tempRevCont, false, 4, 0);
    }
    doRevolution();
    tempRevCont--;
  } else if (tempPauseSeconds == 0) {
    setTempValuables();
    delay(500);
  } else {
    // on display hh:min
    //int hours = tempPauseMinutes / 60;
    //int minutes = tempPauseMinutes % 60;
    //disp.showNumberDecEx(hours, 64, true, 2, 0);
    //disp.showNumberDec(minutes, true, 2, 2);

    // on display min:ss
    int minutes = tempPauseSeconds / 60;
    int seconds = tempPauseSeconds % 60;
    disp.showNumberDecEx(minutes, 64, true, 2, 0);
    disp.showNumberDecEx(seconds, 64, true, 2, 2);

    tempPauseSeconds--;
    if (tempPauseSeconds % 60UL == 0) {
      tempPauseMinutes--;
    }
    delay(470);
  }
}
// **************************************************************************
void doRevolution() {
  stepper.step(revDirection * STEPS * revBig);
  if (revDirectionMode == 0) {
    revDirection = revDirection * -1;
  }
}
// **************************************************************************
void setTempValuables() {
  tempRevCont = revCont;
  tempPauseMinutes = pauseMinutes;
  tempPauseSeconds = pauseMinutes * 60UL;
  tempRevFull = revFull;
}
// **************************************************************************
void setDirection() {
  revDirectionMode++;
  if (revDirectionMode > 2) {
    revDirectionMode = 0;
  }

  if (revDirectionMode == 0) {
    disp.setSegments(dispBothSides, 4, 0);
    revDirection = 1;
  } else if (revDirectionMode == 1) {
    disp.setSegments(dispCw, 4, 0);
    revDirection = -1;
  } else if (revDirectionMode == 2) {
    disp.setSegments(dispCCw, 4, 0);
    revDirection = 1;
  }
  delay(2000);
}
// **************************************************************************
void setRevolutionNumber() {
  revNumberMode++;
  if (revNumberMode > sizeof(revNumbers) / sizeof(revNumbers[0]) - 1) {
    revNumberMode = 0;
  }

  revFull = revNumbers[revNumberMode];
  setTempValuables();
  disp.showNumberDec(revFull, false, 4, 0);
  delay(2000);
}
// **************************************************************************
void doFullTension() {
  if (tempRevFull > 0) {
    if (tempRevFull % 2 == 1) {
      disp.setSegments(dispF, 2, 0);
      if (revDirectionMode == 0) {
        disp.setSegments(dispDirectionMode_0, 2, 2);
      } else if (revDirectionMode == 1) {
        disp.setSegments(dispDirectionMode_0, 2, 2);
      } else if (revDirectionMode == 2) {
        disp.setSegments(dispDirectionMode_0, 2, 2);
      }
    } else {
      disp.showNumberDec(tempRevFull, false, 4, 0);
    }
    doRevolution();
    tempRevFull--;
  } else {
    disp.setSegments(dispDone, 4, 0);
    delay(3000);
    menuStep = 0;
    revDirectionMode = -1;
    revNumberMode = -1;
    contOrFullMode = 0;
  }
}
