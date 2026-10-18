// In v3.0 The syste is equipped with limit switches at +65 and -65 degrees. 
// The system automatically detectes when is reaches a limit position and sets the degrees and steps accordingly. 
//Also equipped with a autocenter function that sets he base to its middle position automatically.
#include <stdlib.h>
#include "esp_system.h"
//#define DEBUG 

// ================= CONFIG =================
#define STEP_PIN 11
#define DIR_PIN  12
#define STEP_DIVISION 8.0

#define LED_FLASH_DELAY 150

#ifdef DEBUG
volatile bool debugLimitSwitch = false;
volatile unsigned long lastDebounceTime = 0; // time of the last interrupt
const unsigned long debounceDelay = 500;   
#endif

const float STEP_ANGLE = 0.09 / STEP_DIVISION;    // degrees per step
const float MIN_ANGLE  = -65.0;
const float MAX_ANGLE  =  65.0;

const int limitSwitch_maxAngle = 5;
const int limitSwitch_minAngle = 13;

const float debounceAngle = 3.0;


const int STEP_DELAY_US = 400;    // motor delay (speed) during  autocenter

const float MAX_DELAY_US = 800.0;
const float MIN_DELAY_US = 300.0;
const float ACC_PROFILE_STEPS = 300.0;

const float a_p = (MAX_DELAY_US - MIN_DELAY_US) / (ACC_PROFILE_STEPS * ACC_PROFILE_STEPS);
const float c_p = MIN_DELAY_US;

// =========================================
volatile bool isMoving = false;
bool stopRequested = false;
volatile bool maxAngleReached = false;
volatile bool minAngleReached = false;
volatile bool angleInterruptHandled = false;


struct __attribute__((aligned(4))) RtcState {
  uint32_t magic;
  float    currentAngle;
  int32_t  stepCount;
};
RTC_NOINIT_ATTR RtcState rtc;

// ----ISRs-------------------------------------
// Interrupt Service Routine for maxAngle
void IRAM_ATTR maxAngleLimitSwitch() {
  maxAngleReached = true;

  #ifdef DEBUG
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > debounceDelay) {
    debugLimitSwitch = true;           
    lastDebounceTime = currentTime;    
  }
  #endif
}

void IRAM_ATTR minAngleLimitSwitch() {
    minAngleReached = true;
  
  #ifdef DEBUG
  unsigned long currentTime = millis();
  if (currentTime - lastDebounceTime > debounceDelay) {
    debugLimitSwitch = true;           
    lastDebounceTime = currentTime;    
  }
  #endif
}

void flash_LED(){
  rgbLedWrite(8, 255, 255, 255);
  delay(LED_FLASH_DELAY);
  rgbLedWrite(8, 0, 0, 0);
  delay(LED_FLASH_DELAY);
  rgbLedWrite(8, 255, 255, 255);
  delay(LED_FLASH_DELAY);
  rgbLedWrite(8, 0, 0, 0);
  delay(LED_FLASH_DELAY);
  rgbLedWrite(8, 255, 255, 255);
  delay(LED_FLASH_DELAY);
  rgbLedWrite(8, 0, 0, 0);
}

// -----------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  pinMode(limitSwitch_maxAngle, INPUT_PULLUP);
  pinMode(limitSwitch_minAngle, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(limitSwitch_maxAngle), maxAngleLimitSwitch, RISING);
  attachInterrupt(digitalPinToInterrupt(limitSwitch_minAngle), minAngleLimitSwitch, RISING);


  if (rtc.magic != 0xCAFEBABE) {
    // cold boot — RTC ram contains garbage
    rtc.currentAngle = 0.0;
    rtc.stepCount = 0;
    rtc.magic = 0xCAFEBABE;
  }
}

// -----------------------------------------

void loop() {
  #ifdef DEBUG
  if (debugLimitSwitch){
    rgbLedWrite(8, 64, 64, 64);
    delay(500);
    rgbLedWrite(8, 0, 0, 0);
    debugLimitSwitch = false;
  }
  
  #endif

  if ((maxAngleReached || minAngleReached) && !angleInterruptHandled){
      //Set angle to max/min angle and steps accordingly
      if (maxAngleReached){
        rgbLedWrite(8, 0, 128, 0);
        rtc.currentAngle = MAX_ANGLE;
        rtc.stepCount = MAX_ANGLE/STEP_ANGLE;
      }
      else if (minAngleReached){
        rgbLedWrite(8, 128, 0, 0);
        rtc.currentAngle = MIN_ANGLE;
        rtc.stepCount = MIN_ANGLE/STEP_ANGLE;
      }
      angleInterruptHandled = true;
    }

  if ((maxAngleReached || minAngleReached) && angleInterruptHandled){
    if (maxAngleReached && (rtc.currentAngle < MAX_ANGLE - debounceAngle)){
      maxAngleReached = false;
      angleInterruptHandled = false;      
      rgbLedWrite(8, 0, 0, 0);
    }
    else if (minAngleReached && (rtc.currentAngle > MIN_ANGLE + debounceAngle)){
      minAngleReached = false;
      angleInterruptHandled = false;      
      rgbLedWrite(8, 0, 0, 0);
    }
     
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();

    handleCommand(cmd);
  }
}

// -----------------------------------------

void handleCommand(String cmd) {

  if (cmd == "HELP") {
    Serial.println("CMDS: GOTO <deg>, MOVE <deg>, POS?, HOME, STOP, HELP");
    return;
  }

  if (cmd == "POS?") {
    Serial.print("POS ");
    Serial.print(rtc.currentAngle, 2);
    Serial.print(" steps=");
    Serial.println(rtc.stepCount);
    return;
  }

  if (cmd == "HOME") {
    if (isMoving) {
      Serial.println("ERROR BUSY");
      return;
    }
    rtc.currentAngle = 0.0;
    rtc.stepCount = 0;
    Serial.println("OK HOMED");
    return;
  }

  if (cmd == "STOP") {
    stopRequested = true;
    Serial.print("OK STOPPED ");
    Serial.println(rtc.currentAngle, 2);
    return;
  }

  if (cmd.startsWith("GOTO ")) {
    if (isMoving) {
      Serial.println("ERROR BUSY");
      return;
    }
    float target = cmd.substring(5).toFloat();
    gotoAngle(target);
    return;
  }

  if (cmd.startsWith("MOVE ")) {
    if (isMoving) {
      Serial.println("ERROR BUSY");
      return;
    }
    float delta = cmd.substring(5).toFloat();
    gotoAngle(rtc.currentAngle + delta);
    return;
  }

  if (cmd == "AUTOCENTER"){
    if (isMoving) {
      Serial.println("ERROR BUSY");
      return;
    }
    autocenter_function();
    return;
  }

  if (cmd == "FLASH LED"){
    if (isMoving) {
      Serial.println("ERROR BUSY");
      return;
    }
    flash_LED();
    Serial.println("OK FLASHED");
    return;
  }
  Serial.println("ERROR UNKNOWN_CMD");
}

// -----------------------------------------

// Computes the delay (in microseconds) for step `i` of a move of `steps` total.
// Replaces the old malloc'd acceleration profile array — same math, no allocation.
float stepDelay(int i, int steps) {
  
  if (steps > 2 * ACC_PROFILE_STEPS) {
    // Long move: accel ramp, plateau, decel ramp
    if (i < ACC_PROFILE_STEPS) {
      // accelerating
      float x = (float)(i - ACC_PROFILE_STEPS);
      return a_p * x * x + c_p;
    } else if (i >= steps - ACC_PROFILE_STEPS) {
      // decelerating (mirror of accel)
      float x = (float)((steps - 1 - i) - ACC_PROFILE_STEPS);
      return a_p * x * x + c_p;
    } else {
      // plateau
      return c_p;
    }
  } else {
    // Short move: triangular profile, no plateau
    float temp_c_p = MAX_DELAY_US - a_p * (float)(steps * steps) / 4.0f;
    float fi = (float)i;
    return a_p * (fi * fi - fi * (float)steps + (float)(steps * steps) / 4.0f) + temp_c_p;
  }
}

// -----------------------------------------

void gotoAngle(float targetAngle) {
  if (targetAngle < MIN_ANGLE || targetAngle > MAX_ANGLE) {
    Serial.println("ERROR OUT_OF_RANGE");
    return;
  }
  Serial.println("OK MOVING");

  // Target position in absolute steps from zero (signed).
  // roundf gives nearest-integer; truncation would bias toward zero.
  int32_t targetStepCount = (int32_t)roundf(targetAngle / STEP_ANGLE);

  // Signed delta in steps from where we are now.
  int32_t deltaSteps = targetStepCount - rtc.stepCount;

  if (deltaSteps == 0) {
    rtc.currentAngle = targetAngle;   // snap UI to requested angle
    Serial.print("OK DONE ");
    Serial.println(rtc.currentAngle, 2);
    return;
  }

  isMoving = true;
  stopRequested = false;

  int direction = (deltaSteps > 0) ? 1 : -1;
  int steps = abs(deltaSteps);    // positive count for the loop

  digitalWrite(DIR_PIN, (direction > 0) ? HIGH : LOW);

  int i = 0;
  for (; i < steps; i++) {
    if (stopRequested) break;

    if ((maxAngleReached || minAngleReached) && !angleInterruptHandled){
      //Set angle to max/min angle and steps accordingly
      if (maxAngleReached){
        rgbLedWrite(8, 0, 128, 0);
        rtc.currentAngle = MAX_ANGLE;
        rtc.stepCount = MAX_ANGLE/STEP_ANGLE;
      }
      else if (minAngleReached){
        rgbLedWrite(8, 128, 0, 0);
        rtc.currentAngle = MIN_ANGLE;
        rtc.stepCount = MIN_ANGLE/STEP_ANGLE;
      }
      
      angleInterruptHandled = true;
      Serial.print("OK DONE ");
      Serial.println(rtc.currentAngle, 2);
      isMoving = false;
      return;
    }

    int d = (int)stepDelay(i, steps);
    #ifdef DEBUB
    Serial.println(d);
    #endif
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(d);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(d);
  }

  // Update the true step count by exactly how many steps we executed.
  int32_t stepsTaken = (int32_t)i * direction;
  rtc.stepCount += stepsTaken;

  isMoving = false;

  
  if (stopRequested) {
    // We stopped partway — report the actual angle from step count.
    rtc.currentAngle = rtc.stepCount * STEP_ANGLE;
    Serial.print("OK STOPPED ");
    Serial.println(rtc.currentAngle, 2);
  } else {
    // Full move completed — UI angle = requested target (clean number),
    // but rtc.stepCount is the truth and has no accumulated drift.
    rtc.currentAngle = targetAngle;
    Serial.print("OK DONE ");
    Serial.println(rtc.currentAngle, 2);
  }

  // if ((maxAngleReached || minAngleReached) && angleInterruptHandled){
  //   if (maxAngleReached && (rtc.currentAngle < MAX_ANGLE - debounceAngle)){
  //     maxAngleReached = false;
  //     angleInterruptHandled = false;      
  //     rgbLedWrite(8, 0, 0, 0);
  //   }
  //   else if (minAngleReached && (rtc.currentAngle > MIN_ANGLE + debounceAngle)){
  //     minAngleReached = false;
  //     angleInterruptHandled = false;      
  //     rgbLedWrite(8, 0, 0, 0);
  //   }
     
  // }
}

void autocenter_function(){
  rgbLedWrite(8,0,0,128);

  digitalWrite(DIR_PIN, HIGH);
  if (minAngleReached){
    for (int i=0; i<abs(debounceAngle/STEP_ANGLE); i++){
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(STEP_DELAY_US);
        digitalWrite(STEP_PIN, LOW);
        delayMicroseconds(STEP_DELAY_US);
      }
  }
  delay(500);

  minAngleReached = false;


  //Go to min edge
  digitalWrite(DIR_PIN, LOW);
  while (!minAngleReached){
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
  delay(500);
  
  //Go to max edge counting steps 
  maxAngleReached = false;
  int step_count = 0;
  digitalWrite(DIR_PIN, HIGH);
  while (!maxAngleReached){
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
    step_count+=1;
  }
  minAngleReached = false;
  delay(500);

  //Go back to middle by dividing steps by 2 
  int step_to_middle = abs(step_count/2);
  digitalWrite(DIR_PIN, LOW);
  for (int i=0; i<step_to_middle;i++){
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(STEP_DELAY_US);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(STEP_DELAY_US);
  }
  maxAngleReached = false;

  rtc.currentAngle = 0.0;
  rtc.stepCount = 0;

  rgbLedWrite(8, 0, 0, 0);
  Serial.print("OK AUTOCENTERED");
  Serial.println(step_count);


}