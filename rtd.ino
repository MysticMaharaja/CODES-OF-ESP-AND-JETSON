// ---------------- PIN DEFINITIONS ----------------
// Analog Inputs 
#define PUSH_POT_PIN   34 
#define BRAKE_POT_PIN  32  
#define APPS_POT_PIN   33 

// Relay Outputs
#define BRAKE_REGEN_RELAY  25 // Controls BOTH Brake Light and Regen
#define RTDS_RELAY         27 // Sounds the buzzer
#define AIR_RELAY          14 // Accumulator Isolation Relay

// ---------------- CONSTANTS & THRESHOLDS ----------------
const int PUSH_THRESHOLD  = 3500; // Values > 3500 = "Pressed" for startup
const int APPS_THRESHOLD  = 800;  // Accelerator threshold 

// --- NEW ACTIVE LOW BRAKE THRESHOLD ---
// Sensor gives High Voltage (reads near 4095) when NOT pressed.
// Sensor gives 0V (reads near 0) when PRESSED.
// We use 2000 as a safe midpoint.
const int BRAKE_PRESSED_THRESHOLD = 2000; 

const unsigned long conditionDelay = 500;
const unsigned long rtdsDuration = 1750;
const unsigned long debugInterval = 200;

// ---------------- GLOBAL VARIABLES ----------------
bool carReady = false; 
bool flag = false;     

// Condition timing
bool conditionTiming = false;
unsigned long conditionStartTime = 0;

// RTDS timing
bool rtdsTriggered = false;
unsigned long rtdsStartTime = 0;

// Debug timing
unsigned long lastDebugTime = 0;

// ---------------- SETUP ----------------
void setup() {
  pinMode(PUSH_POT_PIN, INPUT);
  pinMode(BRAKE_POT_PIN, INPUT);
  pinMode(APPS_POT_PIN, INPUT);

  pinMode(BRAKE_REGEN_RELAY, OUTPUT);
  pinMode(RTDS_RELAY, OUTPUT);
  pinMode(AIR_RELAY, OUTPUT);

  // Initialize relays to OFF
  digitalWrite(BRAKE_REGEN_RELAY, LOW);
  digitalWrite(RTDS_RELAY, LOW);
  digitalWrite(AIR_RELAY, LOW);

  Serial.begin(115200);
  Serial.println("=== Vehicle Analog Control System Initialized ===");
  Serial.println("Note: Brake logic is ACTIVE LOW (Drops to 0V when pressed)");
}

// ---------------- LOOP ----------------
void loop() {
  // --------- READ SENSORS ---------
  int pushVal = analogRead(PUSH_POT_PIN);
  int brakeVal = analogRead(BRAKE_POT_PIN);
  int appsVal = analogRead(APPS_POT_PIN);

  // --------- CONVERT TO LOGIC STATES ---------
  bool pushPressed = (pushVal > PUSH_THRESHOLD);
  
  // LOGIC REVERSED: If value drops BELOW 2000, the brake is considered PRESSED
  bool brakePressed = (brakeVal < BRAKE_PRESSED_THRESHOLD); 
  
  bool appsPressed = (appsVal > APPS_THRESHOLD);

  // --------- BRAKE LIGHT RELAY LOGIC ---------
  if (brakePressed) {
    digitalWrite(BRAKE_REGEN_RELAY, HIGH); // Turn ON brake light
  } else {
    digitalWrite(BRAKE_REGEN_RELAY, LOW);  // Turn OFF brake light
  }

  // --------- STARTUP SEQUENCE (RTDS & AIR) ---------
  // To start: Push > 3500 AND Brake < 2000 AND Accelerator < 800
  bool startCondition = (pushPressed && brakePressed && !appsPressed);

  if (!carReady) {
    if (startCondition) {
      if (!conditionTiming) {
        conditionStartTime = millis();
        conditionTiming = true;
        Serial.println("Start sequence initiated...");
      } 
      else if (millis() - conditionStartTime >= conditionDelay) {
        carReady = true;
        flag = true; 
        
        digitalWrite(AIR_RELAY, HIGH);
        Serial.println("✅ AIR CLOSED - HV SYSTEM ACTIVE | Flag: TRUE");

        digitalWrite(RTDS_RELAY, HIGH);
        rtdsStartTime = millis();
        rtdsTriggered = true;
        Serial.println("🔊 RTDS SOUNDING...");
      }
    } else {
      if (conditionTiming) {
        Serial.println("❌ Start condition broke before delay");
      }
      conditionTiming = false;
    }
  }

  // --------- RTDS TIMER CONTROL ---------
  if (rtdsTriggered) {
    if (millis() - rtdsStartTime >= rtdsDuration) {
      digitalWrite(RTDS_RELAY, LOW);
      rtdsTriggered = false;
      Serial.println("🔇 RTDS OFF - CAR IS READY TO DRIVE");
    }
  }

  // --------- DEBUG PRINT ---------
  if (millis() - lastDebugTime >= debugInterval) {
    lastDebugTime = millis();

    Serial.print("Push:");
    Serial.print(pushVal);
    Serial.print(" Brake:");
    Serial.print(brakeVal);
    Serial.print(" APPS:");
    Serial.print(appsVal);

    Serial.print(" || AIR:");
    Serial.print(carReady ? "CLOSED " : "OPEN ");

    Serial.print(" | Brake Relay:");
    Serial.println(brakePressed ? "ON" : "OFF");
  }

  delay(10); 
}