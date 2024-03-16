#include <TM1637Display.h>
#include <Stepper.h>
#define STEPS 2048  // the number of steps in one revolution of your motor (28BYJ-48)
#define BUTTON 2    //
#define DISP_CLK 6  //
#define DISP_DIO 5  //
#define stepperIn1 9
#define stepperIn2 10
#define stepperIn3 11
#define stepperIn4 3
// **************************************************************************
// ******* VARIABLE DEFINITIONS *********************************************
// **************************************************************************
float revBig = 1.93;  // revolutions factor for one case revolution - 1.93 default
int oneRevolutionTimeSeconds = 12;
// **************************************************************************
// ******** DISPLAY DEFINITIONS *********************************************
// **************************************************************************
uint8_t dispCont[] = { SEG_A | SEG_F | SEG_E | SEG_D, SEG_G | SEG_E | SEG_D | SEG_C, SEG_E | SEG_G | SEG_C, SEG_F | SEG_G | SEG_E | SEG_D };                                  // Cont
uint8_t dispC[] = { SEG_A | SEG_F | SEG_E | SEG_D, 0, 0, 0 };                                                                                                                 // C
uint8_t dispFull[] = { SEG_F | SEG_A | SEG_G | SEG_E, SEG_E | SEG_D | SEG_C, SEG_B | SEG_C, SEG_B | SEG_C };                                                                  // Full
uint8_t dispF[] = { SEG_F | SEG_A | SEG_G | SEG_E, 0, 0, 0 };                                                                                                                 // F
uint8_t dispBothSidesShort[] = { SEG_G | SEG_B | SEG_C, SEG_F | SEG_E | SEG_G, 0, 0 };                                                                                        // -| |-
uint8_t dispCwShort[] = { SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, 0, 0 };                                                                                               // |- |-
uint8_t dispCCwShort[] = { SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, 0, 0 };                                                                                              // -| -|
uint8_t dispDone[] = { SEG_B | SEG_C | SEG_D | SEG_E | SEG_G, SEG_G | SEG_C | SEG_D | SEG_E, SEG_C | SEG_E | SEG_G, SEG_A | SEG_D | SEG_E | SEG_F | SEG_G };                  // donE
uint8_t dispDo[] = { SEG_A | SEG_F | SEG_E | SEG_D, SEG_E | SEG_D | SEG_F, SEG_F | SEG_E, SEG_A | SEG_F | SEG_E | SEG_D };                                                    // CLIC
uint8_t dispCw[] = { SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G };                                                            // |- |- |- |-
uint8_t dispCCw[] = { SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C };                                                           // -| -| -| -|
uint8_t dispBothSides[] = { SEG_G | SEG_B | SEG_C, SEG_G | SEG_B | SEG_C, SEG_F | SEG_E | SEG_G, SEG_F | SEG_E | SEG_G };                                                     // -| -| |- |-
uint8_t dispHelo[] = { SEG_F | SEG_E | SEG_G | SEG_B | SEG_C, SEG_A | SEG_F | SEG_G | SEG_E | SEG_D, SEG_F | SEG_E | SEG_D, SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F };  // HELO
// **************************************************************************
Stepper stepper(STEPS, stepperIn1, stepperIn3, stepperIn2, stepperIn4);
const byte relayStepperVcc = 7;

TM1637Display disp(DISP_CLK, DISP_DIO);

enum MenuSteps { HELLO,
                 MODE,
                 REVOLUTIONS_NUMBER,
                 DIRECTION,
                 TENSION };
enum MenuSteps menuStep;

enum Modes { CONTINOUS,
             FULL };
enum Modes mode;

int revolutions[] = { 40,
                      750,
                      1500,
                      2500 };
int revolution;

enum Directions { BOTH,
                  CW,
                  CCW };
enum Directions direction;

int stepButton;
int waitForStepAcceptTemplate = 5;  // default 5
int waitForStepAccept;
int loopDelaySecondsTemplate = 1;
int pauseInContModeTemplate;
int pauseInContMode;
int continousModeCounter;


// **************************************************************************
void setup() {
  //Serial.begin(9600);

  //wyciszenie silnika krokowego - magia od Adama Śmiałka
  TCCR1B = TCCR1B & B11111000 | B00000001;  // zmiana częstotliwości PWM na 31372.55 Hz (dotyczy portów 9 & 10)
  TCCR2B = TCCR2B & B11111000 | B00000001;  // zmiana częstotliwości PWM na 31372.55 Hz (dotyczy portów 3 & 11)

  stepper.setSpeed(10);
  pinMode(relayStepperVcc, OUTPUT);

  relaysOn(false);

  disp.clear();
  disp.setBrightness(1);
  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), onPushButton, FALLING);

  waitForStepAccept = waitForStepAcceptTemplate;
  disp.setSegments(dispHelo, 4, 0);

  menuStep = HELLO;
  resetVariables();

  pauseInContModeTemplate = (3600 - (oneRevolutionTimeSeconds * revolutions[0])) / loopDelaySecondsTemplate;
  pauseInContMode = pauseInContModeTemplate;
}
// **************************************************************************
void loop() {
  delay(loopDelaySecondsTemplate * 1000);

  if (waitForStepAccept > 0) {
    waitForStepAccept--;
  } else {
    waitForStepAccept = waitForStepAcceptTemplate;
    stepButton = 0;

    if (menuStep == MODE && mode == CONTINOUS) {
      menuStep = DIRECTION;
    } else if (menuStep < TENSION) {
      menuStep = menuStep + 1;
    }
  }

  switch (menuStep) {
    case MODE:
      chooseMode();
      break;
    case REVOLUTIONS_NUMBER:
      chooseRevolutionsNumber();
      break;
    case DIRECTION:
      chooseDirection();
      break;
    case TENSION:
      doRevolution();
      break;
  }

  //Serial.println("menuStep=" + String(menuStep) + ", waitForStepAccept=" + waitForStepAccept + ", mode=" + mode + ", revolution=" + revolution + ", direction=" + direction);
  //Serial.println("pauseInContMode=" + String(pauseInContMode));
}
// **************************************************************************
void onPushButton() {
  static unsigned long lastTime;
  unsigned long timeNow = millis();
  if (timeNow - lastTime < 500) return;

  waitForStepAccept = waitForStepAcceptTemplate;

  stepButton++;

  //Serial.println("stepButton=" + String(stepButton));

  switch (menuStep) {
    case HELLO:
      resetVariables();
      menuStep = MODE;
      break;
    case MODE:
      if (stepButton > FULL) stepButton = 0;
      mode = stepButton;
      break;
    case REVOLUTIONS_NUMBER:
      if (stepButton > sizeof(revolutions) / sizeof(revolutions[0]) - 1) stepButton = 0;
      revolution = revolutions[stepButton];
      break;
    case DIRECTION:
      if (stepButton > CCW) stepButton = 0;
      direction = stepButton;
      break;
    case TENSION:
      menuStep = HELLO;
      disp.setSegments(dispDo, 4, 0);
      //loop();
      break;
  }

  lastTime = timeNow;
}
// **************************************************************************
void doRevolution() {
  if (revolution > 0) {
    relaysOn(true);

    if (revolution % 2 == 1) {
      displayModeChar();
      displayDirectionSymbol();
    } else {
      disp.showNumberDec(revolution, false, 4, 0);
    }

    changeDirectionIfNeeded();

    stepper.step(STEPS * revBig);

    if (menuStep == TENSION) {
      revolution--;
    }
  } else {
    relaysOn(false);

    switch (mode) {
      case CONTINOUS:
        if (pauseInContMode == pauseInContModeTemplate) continousModeCounter++;
        if (continousModeCounter > 999) continousModeCounter = 0;

        if (pauseInContMode > 0) {
          if (pauseInContMode % 30 == 0) {
            displayModeChar();
            disp.showNumberDec(continousModeCounter, false, 3, 1);
          } else {
            int minutes = pauseInContMode / 60;
            int seconds = pauseInContMode % 60;
            disp.showNumberDecEx(minutes, 64, true, 2, 0);
            disp.showNumberDecEx(seconds, 64, true, 2, 2);
          }

          pauseInContMode--;
        } else {
          revolution = revolutions[0];
          pauseInContMode = pauseInContModeTemplate;
        }
        break;
      case FULL:
        menuStep = HELLO;
        resetVariables();
        disp.setSegments(dispDone, 4, 0);
        break;
    }
  }
}
// **************************************************************************
void changeDirectionIfNeeded() {
  switch (direction) {
    case BOTH:
      revBig = revBig * -1;
      break;
    case CW:
      if (revBig < 0) revBig = revBig * -1;
      break;
    case CCW:
      if (revBig > 0) revBig = revBig * -1;
      break;
  }
}

void displayModeChar() {
  switch (mode) {
    case CONTINOUS:
      disp.setSegments(dispC, 2, 0);
      break;
    case FULL:
      disp.setSegments(dispF, 2, 0);
      break;
  }
}

void displayDirectionSymbol() {
  switch (direction) {
    case BOTH:
      disp.setSegments(dispBothSidesShort, 2, 2);
      break;
    case CW:
      disp.setSegments(dispCwShort, 2, 2);
      break;
    case CCW:
      disp.setSegments(dispCCwShort, 2, 2);
      break;
  }
}

void chooseMode() {
  switch (mode) {
    case CONTINOUS:
      disp.setSegments(dispCont, 4, 0);
      break;
    case FULL:
      disp.setSegments(dispFull, 4, 0);
      break;
  }
}

void chooseRevolutionsNumber() {
  disp.showNumberDec(revolution, false, 4, 0);
}

void chooseDirection() {
  switch (direction) {
    case BOTH:
      disp.setSegments(dispBothSides, 4, 0);
      break;
    case CW:
      disp.setSegments(dispCw, 4, 0);
      break;
    case CCW:
      disp.setSegments(dispCCw, 4, 0);
      break;
  }
}

void resetVariables() {
  stepButton = 0;
  mode = CONTINOUS;
  revolution = revolutions[0];
  direction = BOTH;
  continousModeCounter = 0;
}

void relaysOn(bool state) {
  if (state) {
    digitalWrite(relayStepperVcc, HIGH);
  } else {
    digitalWrite(relayStepperVcc, LOW);
  }
}
