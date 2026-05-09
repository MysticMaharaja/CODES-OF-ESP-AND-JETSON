// ==========================================
// ESP32 - BAJA STANDALONE TEST SEQUENCE
//
// FINAL TEST SEQUENCE
//
// PHASE 0:
// Release Brake
// Relay19 ON
// Relay18 OFF
// Duration: 5 sec
//
// PHASE 1:
// Ramp throttle to 75%
// Duration: 3 sec
//
// PHASE 2:
// Hold 75% throttle
// Duration: 10 sec
//
// PHASE 3:
// INSTANT return to idle voltage (0.8V)
// Apply brake
// Relay18 ON
// Relay19 OFF
// Regen Relay ON
// Duration: 5 sec
//
// PHASE 4:
// DONE / SAFE HOLD
//
// IMPORTANT:
// BOTH RELAYS OFF = actuator unpowered
// ==========================================



// ==========================================
// PIN DEFINITIONS
// ==========================================

// PWM throttle output
const int converterPwmPin = 25;

// Relay pins
const int relay18 = 18;   // EXTEND actuator = BRAKE APPLY
const int relay19 = 19;   // RETRACT actuator = BRAKE RELEASE

// Regenerative braking relay
const int regenRelayPin = 13;



// ==========================================
// PWM SETTINGS
// ==========================================

const int pwmFreq       = 1000;
const int pwmResolution = 8;   // 0-255



// ==========================================
// CALIBRATED PWM VALUES
// ==========================================

const int dutyIdle = 10;   // ~0.8V
const int dutyMax  = 113;  // ~4.2V



// ==========================================
// USER-TUNABLE PARAMETERS
// ==========================================

// Final target throttle %
const int TARGET_THROTTLE_PCT = 91.5;


// ----- PHASE TIMINGS -----

// Time to retract actuator before moving
const unsigned long BRAKE_RELEASE_TIME_MS = 5000;

// Time to ramp from idle -> target throttle
const unsigned long THROTTLE_RAMP_TIME_MS = 3500;

// Time to hold target throttle
const unsigned long THROTTLE_HOLD_TIME_MS = 8000;

// Time to keep brake actuator powered
const unsigned long BRAKE_APPLY_TIME_MS = 10000;



// ==========================================
// PWM RAMP SETTINGS
// ==========================================

const int RAMP_STEP_UP      = 1;
const int RAMP_STEP_DOWN    = 2;

const unsigned long RAMP_INTERVAL_MS = 5;



// ==========================================
// TEST STATE MACHINE
// ==========================================

enum TestState {

  STATE_BRAKE_RELEASE,
  STATE_THROTTLE_RAMP,
  STATE_THROTTLE_HOLD,
  STATE_BRAKE_APPLY,
  STATE_DONE
};

TestState currentState = STATE_BRAKE_RELEASE;

unsigned long stateStart = 0;



// ==========================================
// PWM VARIABLES
// ==========================================

int currentDuty = 0;
int targetDuty  = dutyIdle;

unsigned long lastRampTick = 0;



// ==========================================
// HELPER FUNCTIONS
// ==========================================



// ==========================================
// SAFE RELAY CONTROL
// ==========================================

void setRelays(bool relay18State, bool relay19State) {

  // Safety check
  if (relay18State && relay19State) {

    digitalWrite(relay18, LOW);
    digitalWrite(relay19, LOW);

    Serial.println("!!! SAFETY BLOCK: BOTH RELAYS REQUESTED !!!");

    return;
  }

  // Break-before-make protection
  digitalWrite(relay18, LOW);
  digitalWrite(relay19, LOW);

  delay(20);

  digitalWrite(relay18, relay18State ? HIGH : LOW);
  digitalWrite(relay19, relay19State ? HIGH : LOW);
}



// ==========================================
// RELEASE BRAKE
// ==========================================

void releaseBrake() {

  Serial.println(">> BRAKE RELEASE");
  Serial.println("   Relay19 ON | Relay18 OFF");

  // Retract actuator
  setRelays(LOW, HIGH);
}



// ==========================================
// APPLY BRAKE
// ==========================================

void applyBrake() {

  Serial.println(">> APPLY BRAKE");
  Serial.println("   Relay18 ON | Relay19 OFF");
  Serial.println("   REGEN RELAY ON");

  // Extend actuator
  setRelays(HIGH, LOW);

  // Turn ON regenerative braking relay
  digitalWrite(regenRelayPin, HIGH);
}



// ==========================================
// STOP ACTUATOR
// ==========================================

void stopActuator() {

  digitalWrite(relay18, LOW);
  digitalWrite(relay19, LOW);

  // Turn OFF regenerative relay
  digitalWrite(regenRelayPin, LOW);

  Serial.println(">> ACTUATOR POWER OFF");
  Serial.println(">> REGEN RELAY OFF");
}



// ==========================================
// THROTTLE % -> PWM DUTY
// ==========================================

int throttleToDuty(int pct) {

  pct = constrain(pct, 0, 100);

  return map(pct, 0, 100, dutyIdle, dutyMax);
}



// ==========================================
// SMOOTH PWM RAMP
// ==========================================

void rampTick() {

  if (millis() - lastRampTick < RAMP_INTERVAL_MS)
    return;

  lastRampTick = millis();


  // Ramp UP
  if (currentDuty < targetDuty) {

    currentDuty += RAMP_STEP_UP;

    if (currentDuty > targetDuty)
      currentDuty = targetDuty;
  }

  // Ramp DOWN
  else if (currentDuty > targetDuty) {

    currentDuty -= RAMP_STEP_DOWN;

    if (currentDuty < targetDuty)
      currentDuty = targetDuty;
  }

  currentDuty = constrain(currentDuty, 0, dutyMax);

  ledcWrite(converterPwmPin, currentDuty);
}



// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  delay(500);


  // ----------------------------------------
  // Configure outputs
  // ----------------------------------------

  pinMode(relay18, OUTPUT);
  pinMode(relay19, OUTPUT);
  pinMode(regenRelayPin, OUTPUT);

  digitalWrite(relay18, LOW);
  digitalWrite(relay19, LOW);
  digitalWrite(regenRelayPin, LOW);


  // ----------------------------------------
  // PWM setup
  // ----------------------------------------

  ledcAttach(converterPwmPin, pwmFreq, pwmResolution);

  ledcWrite(converterPwmPin, dutyIdle);


  // ----------------------------------------
  // Startup Information
  // ----------------------------------------

  Serial.println("========================================");
  Serial.println("      BAJA TEST SEQUENCE");
  Serial.println("========================================");

  Serial.printf("Target Throttle     : %d%%\n",
                TARGET_THROTTLE_PCT);

  Serial.printf("Brake Release Time  : %lu ms\n",
                BRAKE_RELEASE_TIME_MS);

  Serial.printf("Throttle Ramp Time  : %lu ms\n",
                THROTTLE_RAMP_TIME_MS);

  Serial.printf("Throttle Hold Time  : %lu ms\n",
                THROTTLE_HOLD_TIME_MS);

  Serial.printf("Brake Apply Time    : %lu ms\n",
                BRAKE_APPLY_TIME_MS);

  Serial.println("========================================");

  Serial.println("Starting in 3 seconds...");

  delay(3000);


  // ========================================
  // START PHASE 0
  // ========================================

  Serial.println("\n[PHASE 0] RELEASE BRAKE");

  releaseBrake();

  currentDuty = dutyIdle;
  targetDuty  = dutyIdle;

  stateStart = millis();

  currentState = STATE_BRAKE_RELEASE;
}



// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

  rampTick();

  unsigned long elapsed = millis() - stateStart;


  switch (currentState) {


    // ======================================
    // PHASE 0
    // RELEASE BRAKE
    // ======================================

    case STATE_BRAKE_RELEASE:

      if (elapsed >= BRAKE_RELEASE_TIME_MS) {

        // Remove actuator power before motion
        stopActuator();

        Serial.println("\n[PHASE 1] THROTTLE RAMP");

        stateStart = millis();

        currentState = STATE_THROTTLE_RAMP;
      }

      break;



    // ======================================
    // PHASE 1
    // RAMP TO TARGET THROTTLE
    // ======================================

    case STATE_THROTTLE_RAMP:

      if (elapsed < THROTTLE_RAMP_TIME_MS) {

        unsigned long currentPct =
          (elapsed * TARGET_THROTTLE_PCT)
          / THROTTLE_RAMP_TIME_MS;

        targetDuty = throttleToDuty(currentPct);
      }

      else {

        targetDuty =
          throttleToDuty(TARGET_THROTTLE_PCT);

        Serial.println("\n[PHASE 2] HOLD THROTTLE");

        stateStart = millis();

        currentState = STATE_THROTTLE_HOLD;
      }

      break;



    // ======================================
    // PHASE 2
    // HOLD THROTTLE
    // ======================================

    case STATE_THROTTLE_HOLD:

      targetDuty =
        throttleToDuty(TARGET_THROTTLE_PCT);

      if (elapsed >= THROTTLE_HOLD_TIME_MS) {

        Serial.println("\n[PHASE 3] APPLY BRAKE");


        // ==================================
        // INSTANT RETURN TO IDLE VOLTAGE
        // ==================================

        currentDuty = dutyIdle;
        targetDuty  = dutyIdle;

        ledcWrite(converterPwmPin, dutyIdle);

        Serial.println(">> THROTTLE RETURNED TO IDLE");


        // ==================================
        // APPLY BRAKE + REGEN
        // ==================================

        applyBrake();

        stateStart = millis();

        currentState = STATE_BRAKE_APPLY;
      }

      break;



    // ======================================
    // PHASE 3
    // APPLY BRAKE
    // ======================================

    case STATE_BRAKE_APPLY:

      if (elapsed >= BRAKE_APPLY_TIME_MS) {

        currentDuty = dutyIdle;
        targetDuty  = dutyIdle;

        ledcWrite(converterPwmPin, dutyIdle);

        stopActuator();

        Serial.println("\n[DONE]");
        Serial.println("Throttle at IDLE");
        Serial.println("Actuator OFF");
        Serial.println("Regen OFF");

        currentState = STATE_DONE;
      }

      break;



    // ======================================
    // DONE
    // ======================================

    case STATE_DONE:

      // Hold safe state

      break;
  }



  // ========================================
  // SERIAL STATUS
  // ========================================

  static unsigned long lastPrint = 0;

  if (millis() - lastPrint > 200) {

    lastPrint = millis();

    const char* stateNames[] = {

      "BRAKE_RELEASE",
      "THROTTLE_RAMP",
      "THROTTLE_HOLD",
      "BRAKE_APPLY",
      "DONE"
    };

    float approxV =
      0.8f +
      ((float)(currentDuty - dutyIdle) /
      (float)(dutyMax - dutyIdle)) * 3.4f;

    if (currentDuty <= dutyIdle)
      approxV = 0.8f;

    Serial.printf(
      "State: %-16s | PWM: %3d | ~%.2fV\n",
      stateNames[currentState],
      currentDuty,
      approxV
    );
  }
}