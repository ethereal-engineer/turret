#include <EnableInterrupt.h>  // Cleaner ways to do things
#include <SD.h>
#include <TMRpcm.h>           // Audio library

#define __ASSERT_USE_STDERR

#include <assert.h>

// To debug or not to debug? - that, is the question
#define DEBUG

// Debug macro
#ifdef DEBUG
  #define dbg(x) Serial.print(__LINE__); Serial.print(": "); Serial.print(x)
  #define dbg2(x, y) dbg(x); Serial.print(y)
  #define dbg3(x, y, z) dbg2(x, y); Serial.print(z)
  #define dbgln(x) dbg(x); Serial.println("")
  #define dbgln2(x,y) dbg2(x, y); Serial.println("")
  #define dbgln3(x, y, z) dbg3(x, y, z); Serial.println("")
#else
  #define dbg(x) void(x)
  #define dbg2(x, y) dbg(x); void(y)
  #define dbg3(x, y, z) dbg2(x, y); void(z)
  #define dbgln(x) dbg(x)
  #define dbgln2(x,y) dbg2(x, y)
  #define dbgln3(x, y, z) dbg3(x, y, z)
#endif

// Helper debugging macro
#define repeat(n, x) for (int _i = 0; _i < n; _i++) { x; }

// Helper macro for readability
#define MINS_TO_MS(mins) mins * 60000

// There might be a proper one - check later
#define MAX_UNSIGNED_LONG 0xFFFFFFFF
#define MAX_PATH_LENGTH 16
// Minimum light level reading that keeps us in day mode (0-1024)
#define MINIMUM_DAY_MODE_AMBIENT_LIGHT_LEVEL 100

/* Pins */

// SD Card (stores the audio clips)

#define PIN_SD_CHIP_SELECT 10
#define PIN_SD_DATA_IN 11
#define PIN_SD_DATA_OUT 12
#define PIN_SD_CLOCK 13

// RGB LED Eye

#define PIN_RGB_EYE_DATA 5
#define PIN_RGB_EYE_CLOCK 6

// Speaker

#define PIN_AUDIO_OUT 9

// Lights

#define PIN_LIGHTS 3

// PIR

#define PIN_MOTION_DETECTOR 2

// CDS/LRD-based ambient light level input

#define PIN_ANALOG_AMBIENT_LIGHT A0

// Unused PIN for random seed generation

#define PIN_UNUSED_ANALOG_FOR_RANDOM_SEED A1

// Classes (split files later)

/* RGBLED - A quick class for driving RGB LEDs via a WS-something chip */
/* The WS-Whatever chip can handle data transfer speeds of up to 25MHz, so no delays are needed during data transmission */

#define SIGNAL_DATA_DELAY 0.5

class RGBLED {

  private:
  
  short _pinClock, _pinData;

  // Private Functions

  /* prepareForData()
   * The WS-whatever chip requires the clock to drop low for this period to signal new data */

  void prepareForData() {
    digitalWrite(_pinClock, LOW);
    delay(SIGNAL_DATA_DELAY);
  }
  
  void cycleClock(){
    digitalWrite(_pinClock, LOW);
    digitalWrite(_pinClock, HIGH);
  }

  public:

  void begin(short pinData, short pinClock) {
    _pinData = pinData;
    _pinClock = pinClock;
    pinMode(_pinData, OUTPUT);
    pinMode(_pinClock, OUTPUT);
  }

  void setColor(unsigned long color) {
    prepareForData();
    for (int i = 23; i >= 0; i--) {
      digitalWrite(_pinData, bitRead(color, i));
      cycleClock();
    }
    dbgln2(F("Set colour to: "), color);
  }
  
};

/* Mosfet-controlled variable power device */

/* The PWM outputs generated on pins 5 and 6 will have higher-than-expected duty cycles. 
   This is because of interactions with the millis() and delay() functions, which share 
   the same internal timer used to generate those PWM outputs. This will be noticed 
   mostly on low duty-cycle settings (e.g 0 - 10) and may result in a value of 0 not 
   fully turning off the output on pins 5 and 6. */
   
class PWMPoweredDevice {
  
  private:
  
  short _pin;        // control pin connected to mosfet gate
  float _power;      // value between 0 and 1 - fractional power
  bool  _on;         // allows power level to be kept and restored on and off

  void updatePWM() {
    short dutyCycle = round(_power * 255) * _on;
    analogWrite(_pin, dutyCycle);
    dbgln3(F("analogWrite to pin "), _pin, dutyCycle);
  }

  public:

  void begin(short pin) {
    _pin = pin;
    _power = 1.0;
    turnOff();
  }

  void turnOn() {
    if (_on) { return; }
    _on = true;
    updatePWM();
  }

  void turnOff() {
    if (!_on) { return; }
    _on = false;
    updatePWM();
  }

  bool isOn() {
    return _on;
  }

  float power() {
    return _power;
  }

  void setPower(float power) {
    _power = max(min(power, 1), 0);
    updatePWM();
  }
};

/* Enums */

/* TurretState - the states of the turret's main finite state automata */

enum TurretState {
  tsInitialising,   // Starting up
  tsSleeping,       // Waiting for prey
  tsActive,         // Target acquired
  tsSearching,      // Searching for lost target
  tsAutoSearching,   // Anybody there?
  tsCount           // Not a state - a count of number of states ;) (there might be a better way)
} turretState = tsInitialising;

/* OperationMode - different modes depending on certain conditions or options */

enum OperationMode {
  omUnknown,
  omNight,
  omDay
} operationMode = omUnknown;

/* EyeColor - colours of the eye for each state or mode or action taken 
   (these aren't linked DIRECTLY to TurretState because we may want to later
    deviate based on day and night etc) - writing this in the car on the way to
    Braidwood Grandparents Day right now. Loving it! */

enum EyeColor {
  ecInitialising = 0xFFFFFF, // A rainbow blotch - it's pretty, but good to see all the LEDs are working
  ecSleeping = 0x222222, // A dim rainbow blotch
  ecActive = 0xFF0000, // Hot alert alarm red
  ecSearching = 0x0000FF, // Blue - are you still there?
  ecAutoSearching = 0xFF00FF // Lonely purple
};

/* Globals */

TMRpcm            audio;  // Audio output using PWM
RGBLED            eye;    // RGB LED Eye to set the mood
PWMPoweredDevice  lights; // "Payload" or "product", as the turret calls it - very bright lights

// Time references use internal millis() counter (since last restart) - which should be enough...?

unsigned long timeOfLastStateChange     = MAX_UNSIGNED_LONG; // Used to check how long we've been in this state
unsigned long timeOfLastMotionDetection = MAX_UNSIGNED_LONG; // Used to check how long since last motion was detected

// TurretState to Sound Path Array
const char soundPaths[][tsCount] PROGMEM = {
  "/",
  "/RETIRE/",
  "/ACTIVE/",
  "/SEARCH/",
  "/AUTOSEARCH/"
};

// Other support sounds
const char soundFxActive[]  PROGMEM = "/FX/ACTIVE/";
const char soundFxAlert[]   PROGMEM = "/FX/ALERT/";
const char soundFxDeploy[]  PROGMEM = "/FX/DEPLOY/";
const char soundFxPing[]    PROGMEM = "/FX/PING/";
const char soundFxRetract[] PROGMEM = "/FX/RETRACT/";

// Support Functions

// handle diagnostic information given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}

/* asciiToHex - Convert single ascii lowercase characters into hex value shorts */

short asciiToHex(char ascii) {
  
  if (ascii < 48) { return 0; }
  if (ascii > 102) { return 0; }
  if (ascii > 57 && ascii < 97) { return 0; }

  if (ascii < 97) {
    // numbers
    return ascii - 48;    
  } else {
    // letters
    return ascii - 87;
  }
}

/* eyeColorForState - map turret states to eye colour */
EyeColor eyeColorForState(TurretState state) {
  switch (state) {
    case tsInitialising:  return ecInitialising;
    case tsSleeping:      return ecSleeping;
    case tsActive:        return ecActive;
    case tsSearching:     return ecSearching;
    case tsAutoSearching: return ecAutoSearching;
    case tsCount:         return ecInitialising; // CRUD
  }
  return ecInitialising; // CRUD
}

/* State timeout durations in milliseconds */

unsigned long stateTimeout[tsCount] = {
    MINS_TO_MS(0),    // tsInitialising
    MINS_TO_MS(15),   // tsSleeping
    MINS_TO_MS(5),    // tsActive
    MINS_TO_MS(1),    // tsSearching   
    MINS_TO_MS(5),    // tsAutoSearching
};

/* randomize - Shuffle up our random order */

void randomize() {
  randomSeed(analogRead(PIN_UNUSED_ANALOG_FOR_RANDOM_SEED));
  dbgln(F("Randomised OK"));
}

/* Returns a random full file path from a base path on the SD card */

void getRandomFilenameAtPath(const char *path, char *shortFileName) {
  File directory = SD.open(path, FILE_READ);
  if (!directory) {
    Serial.print("ERROR: Unable to open path ");
    Serial.println(path); 
    assert(false); // Bail
  }

  short fileCount = 0;

  // Optimised for least amount of dynamic allocation (a bit yuck),
  // we run through once to get the count, and then a second time
  // to grab our filename.
  
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) { break; } // No more files
    if (entry.isDirectory()) { continue; }; // Skip subdirectories
    fileCount++;
    entry.close();
  }

  if (fileCount == 0) {
    Serial.print("ERROR: No files at path: ");
    Serial.println(path); 
    assert(false); // Bail
  }

  dbgln2(F("Found valid files numbering: "), fileCount);

  // Rewind and start the file listing again
  directory.rewindDirectory();

  // The index of choice is...
  short index = random() % fileCount;

  dbgln2(F("Random file index choice is: "), index);

  // Skip all the files we don't want
  for (int i = 0; i < index; i++) {
    File entry = directory.openNextFile();
    if (!entry) {
      Serial.print("ERROR: Ran out of files while seeking #");
      Serial.print(index);
      Serial.print(" at path: ");
      Serial.println(path); 
      assert(false); // Bail
    }
    if (entry.isDirectory()) { continue; }; // Skip subdirectories
    entry.close();
  }

  // Okay - we're up to the file we actually want info on
  File randomEntry = directory.openNextFile();
  if (!randomEntry) {
      Serial.print("ERROR: Couldn't open the chosen file while seeking #");
      Serial.print(index);
      Serial.print(" at path: ");
      Serial.println(path); 
      assert(false); // Bail
  }

  dbgln2(F("Chose random file: "), randomEntry.name());

  

  strcpy(shortFileName, randomEntry.name());
  
  randomEntry.close();
  
  directory.rewindDirectory();
  directory.close();
}

void resetStateChangeTimer() {
  timeOfLastStateChange = millis();
  dbgln(F("State change timer was reset"));
}

// Playback Functions

void playSound(const char *name) {
  dbgln2(F("Beginning playback of sound: "), name);
  audio.play((char *)name);
  while (audio.isPlaying()) {
    delay(100);
  }
  dbgln2(F("Finished playback of sound: "), name);
}

void playRandomSoundForState(TurretState state) {
  char shortFileName[12];
  const char *path = (const char *)soundPaths[state];
  for (int i= 0; i<tsCount; i++) {
    dbgln(soundPaths[i]);
  }
  getRandomFilenameAtPath(path, &shortFileName[0]);
  // Concat then play
  char fullFilePath[MAX_PATH_LENGTH];
  strcpy(&fullFilePath[0], path);
  strcat(fullFilePath, shortFileName);
  dbgln2(F("Will play random sound with full path: "), fullFilePath);
  playSound(fullFilePath);
}

// Ambient Light Level Measurement

unsigned int getAmbientLightLevel() {
  unsigned int level = analogRead(PIN_ANALOG_AMBIENT_LIGHT);
  dbgln2(F("Got ambient light level: "), level);
  return level;
}

// Callback Functions

void motionWasDetected() {
  // Update the time of last motion detected
  // This routine uses interrupts so no Serial/dbg use in here!
  timeOfLastMotionDetection = millis();
}

void turretStateWillChange(TurretState toState) {
  // Play preparation sounds
  if (toState == tsActive) {
    playSound(soundFxActive);
  }
  if (turretState == tsSleeping) {
    playSound(soundFxDeploy);
  }
}

void turretStateDidChange(TurretState fromState) {
  // Update the time of last state change
  resetStateChangeTimer();
  // Update eye colour to new state colour
  eye.setColor(eyeColorForState(turretState));
  // Play new state sound
  playRandomSoundForState(turretState);
  // If state is sleep, play retract fx and turn off any lights
  if (turretState == tsSleeping) {
    lights.turnOff();
    playSound(soundFxRetract);
  }
  // If state is active and it's night time,
  // light 'em up!
  if (turretState == tsActive && operationMode == omNight) {
    lights.turnOn();
  }
}

void operationModeWillChange(OperationMode toMode) {
  // At this point, we randomise our audio responses
  randomize();
}

void operationModeDidChange(OperationMode fromMode) {
  
}

// Mode Change Functions

bool shouldTimeoutSansMotion() {
  bool doTimeout = (millis() - timeOfLastStateChange > stateTimeout[turretState]);
  if (doTimeout) {
    dbgln(F("Turret state timeout reached"));
  }
  return doTimeout;
}

void enterTurretState(TurretState state) {
  if (state == turretState) { return; }
  TurretState old = turretState;
  dbgln3(F("Turret state will change to "),state, old);
  turretStateWillChange(state);
  turretState = state;
  turretStateDidChange(old);
  dbgln3(F("Turret state did change to "),state, old);
}

void enterOperationMode(OperationMode mode) {
  if (mode == operationMode) { return; }
  OperationMode old = operationMode;
  dbgln3(F("Operation mode will change to "),mode ,old);
  operationModeWillChange(mode);
  operationMode = mode;
  operationModeDidChange(old);
  dbgln3(F("Operation mode did change to "),mode ,old);
}

// Main

void setup(){
  // Debugging
  Serial.begin(9600);
  // Configure "THE EYE"
  eye.begin(PIN_RGB_EYE_CLOCK, PIN_RGB_EYE_DATA);
  // Configure audio output
  audio.speakerPin = PIN_AUDIO_OUT; // PWM output is pin 9 for a UNO
  audio.quality(1);                 // Use oversampling function for best quality
  // Configure the "product"
  lights.begin(PIN_LIGHTS);
  // Configure the PIR
  pinMode(PIN_MOTION_DETECTOR, INPUT_PULLUP);
  enableInterrupt(PIN_MOTION_DETECTOR, motionWasDetected, RISING);
  // Configure SD Card
  if (!SD.begin(PIN_SD_CHIP_SELECT)) {
    Serial.println(F("ERROR: Unable to initialise SD card.")); 
    assert(false); // Bail
  }
}

void loop(){  
  // Continuously looping and evaluating which state to enter
  if (getAmbientLightLevel() >= MINIMUM_DAY_MODE_AMBIENT_LIGHT_LEVEL) {
    // add a delay to how often we can change between day and night modes if needed
    enterOperationMode(omDay);
  } else {
    enterOperationMode(omNight);
  }

  #ifdef DEBUG
    static unsigned long mostRecentMotionDetected = MAX_UNSIGNED_LONG;
    if (mostRecentMotionDetected != timeOfLastMotionDetection) {
      mostRecentMotionDetected = timeOfLastMotionDetection;
      dbgln2(F("Motion has been detected at millis = "),timeOfLastMotionDetection);
    }
  #endif

  switch (turretState) {
    case tsInitialising:
      // What to do here...?
      // Start asleep
      enterTurretState(tsSleeping);
      break;
    case tsSleeping:
      // If motion detected since state change, enter active state
      if (timeOfLastStateChange < timeOfLastMotionDetection) {
        enterTurretState(tsActive);
        break;
      } 
      // If we're in night mode, there's nothing to do because we don't autosearch
      // at night
      if (operationMode == omNight) { 
        break; 
      }
      // if no motion for a long time (and not in night mode), enter autosearching state
      if (shouldTimeoutSansMotion()) {
        enterTurretState(tsAutoSearching);
      }
      break;
    case tsActive:
      // If motion is detected, ping to confirm
      if (timeOfLastStateChange < timeOfLastMotionDetection) {
        playSound(soundFxPing);
        resetStateChangeTimer(); // Essentially re-entering the active state without sound or colour changes...
        break;
      } 
      // If no motion detected for a while, enter searching state
      if (shouldTimeoutSansMotion()) {
        enterTurretState(tsSearching);
      }
      break;
    case tsSearching:
      // If motion detected enter active state
      if (timeOfLastStateChange < timeOfLastMotionDetection) {
        enterTurretState(tsActive);
        break;
      } 
      // If no motion detected for a while DEF, enter sleep state
      if (shouldTimeoutSansMotion()) {
        enterTurretState(tsSleeping);
      }
      break;
    case tsAutoSearching:
      // If motion detected enter active state
      if (timeOfLastStateChange < timeOfLastMotionDetection) {
        enterTurretState(tsActive);
        break;
      } 
      // If no motion detected for a while DEF, enter sleep state
      if (shouldTimeoutSansMotion()) {
        enterTurretState(tsSleeping);
      }
      break;
    case tsCount: //CRUD - fixme
      break;
  }

}
