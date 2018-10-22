#include <EnableInterrupt.h>  // Cleaner ways to do things
#include <SD.h>
#include <TMRpcm.h>           // Audio library

#define __ASSERT_USE_STDERR

#include <assert.h>

// Fix an existing bug in definitions somewhere
#ifndef FPSTR
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#endif

// To debug or not to debug? - that, is the question
#define DEBUG

// Debug settings panel
#ifdef DEBUG
  //#define LOG_AMBIENT_LIGHT_LEVEL
  //#define USE_SHORT_TIMEOUTS
#endif

// Debug logging macro
#ifdef DEBUG
  #define dbg(x) Serial.print(__LINE__); Serial.print(": "); Serial.print(x)
  #define dbg2(x, y) dbg(x); Serial.print(" "); Serial.print(y)
  #define dbg3(x, y, z) dbg2(x, y); Serial.print(" "); Serial.print(z)
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

// Main loop delay
#define MAIN_LOOP_DELAY 500

// Helper debugging macro
#define repeat(n, x) for (int _i = 0; _i < n; _i++) { x; }

// Helper macros for readability
#define MINS_TO_MS(mins) mins * 60000
#define SECS_TO_MS(secs) secs * 1000

// There might be a proper one - check later
#define MAX_UNSIGNED_LONG 0xFFFFFFFF
#define MAX_UNSIGNED_INT 0xFFFF
#define MAX_PATH_LENGTH 16
// Minimum light level reading that keeps us in day mode (0-1024)
#define MINIMUM_DAY_MODE_AMBIENT_LIGHT_LEVEL 100

// Light level override for debugging
#ifdef DEBUG
  unsigned int ambientLightLevelOverride = MAX_UNSIGNED_INT;
#endif

/* Pins */

// SD Card (stores the audio clips)

#define PIN_SD_CHIP_SELECT 10 // Physical Pin 16
#define PIN_SD_DATA_IN 11     // Physical Pin 17
#define PIN_SD_DATA_OUT 12    // Physical Pin 18
#define PIN_SD_CLOCK 13       // Physical Pin 19

// RGB LED Eye

#define PIN_RGB_EYE_DATA 5    // Physical Pin 11
#define PIN_RGB_EYE_CLOCK 6   // Physical Pin 12

// Speaker

#define PIN_AUDIO_OUT 9       // Physical Pin 15

// Lights

#define PIN_LIGHTS 3          // Physical Pin 5

// PIR

#define PIN_MOTION_DETECTOR 2 // Physical Pin 4

// CDS/LRD-based ambient light level input

#define PIN_ANALOG_AMBIENT_LIGHT A0           // Physical Pin 23

// Unused PIN for random seed generation

#define PIN_UNUSED_ANALOG_FOR_RANDOM_SEED A1  // Physical Pin 24

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

// TODO: filter out noise from LED PWM. Until then, use full-mix colours

enum EyeColor {
  ecInitialising = 0xFFFFFF, // A rainbow blotch - it's pretty, but good to see all the LEDs are working
  ecSleeping = 0xFFFFFF, // A rainbow blotch
  ecActive = 0xFF0000, // Hot alert alarm red
  ecSearching = 0xFF00FF, // Purple - are you still there?
  ecAutoSearching = 0x0000FF // Lonely blue
};

/* Globals */

TMRpcm            audio;  // Audio output using PWM
RGBLED            eye;    // RGB LED Eye to set the mood
PWMPoweredDevice  lights; // "Payload" or "product", as the turret calls it - very bright lights

// Time references use internal millis() counter (since last restart) - which should be enough...?

unsigned long timeOfLastStateChange     = MAX_UNSIGNED_LONG; // Used to check how long we've been in this state
unsigned long timeOfLastMotionDetection = 0; // Used to check how long since last motion was detected

// Sound Path strings
static const char soundPathNone[]       PROGMEM = "/";
static const char soundPathRetire[]     PROGMEM = "/RETIRE/";
static const char soundPathActive[]     PROGMEM = "/ACTIVE/";
static const char soundPathSearch[]     PROGMEM = "/SEARCH/";
static const char soundPathAutoSearch[] PROGMEM = "/AUTOSE~1/";
static const char soundPathDeploy[]     PROGMEM = "/DEPLOY/"; // Not linked to a state

// TurretState to Sound Path Array
const char * const soundPaths[] PROGMEM = {
  soundPathNone,
  soundPathRetire,
  soundPathActive,
  soundPathSearch,
  soundPathAutoSearch
};

// Other support sounds
static const char soundFxActive[]  PROGMEM = "/FX/ACTIVE";
static const char soundFxAlert[]   PROGMEM = "/FX/ALERT";
static const char soundFxDeploy[]  PROGMEM = "/FX/DEPLOY";
static const char soundFxPing[]    PROGMEM = "/FX/PING";
static const char soundFxRetract[] PROGMEM = "/FX/RETRACT";

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

#ifdef USE_SHORT_TIMEOUTS

unsigned long stateTimeout[tsCount] = {
    0,    // tsInitialising
    30000,   // tsSleeping
    30000,    // tsActive
    15000,    // tsSearching   
    10000,    // tsAutoSearching
};

#else

unsigned long stateTimeout[tsCount] = {
    MINS_TO_MS(0),    // tsInitialising
    MINS_TO_MS(15),   // tsSleeping
    MINS_TO_MS(5),    // tsActive
    MINS_TO_MS(1),    // tsSearching   
    SECS_TO_MS(10),   // tsAutoSearching
};

#endif

/* A special reset function for debugging etc */
void(* resetFunc) (void) = 0; // declare reset function @ address 0

/* randomize - Shuffle up our random order */

void randomize() {
  dbgln(F("Initialising randomiser..."));
  unsigned long randomSeedValue = analogRead(PIN_UNUSED_ANALOG_FOR_RANDOM_SEED) + getAmbientLightLevel() + millis();
  randomSeed(randomSeedValue);
  for (int i = 0; i < randomSeedValue % 10; i++) {
    Serial.print(random(0,100));
  }
  Serial.println("");
  dbgln2(F("Randomised with value: "), randomSeedValue);
}

/* Returns a random full file path from a base path on the SD card */

void getRandomFilenameAtPath(const char *path, char *shortFileName) {
  
  dbgln2(F("Getting random filename at path "), path);
  
  File directory = SD.open(path, FILE_READ);
  if (!directory) {
    Serial.print(F("ERROR: Unable to open path "));
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
    Serial.print(F("ERROR: No files at path: "));
    Serial.println(path); 
    assert(false); // Bail
  }

  dbgln2(F("Found valid files numbering: "), fileCount);

  // Rewind and start the file listing again
  directory.rewindDirectory();

  // The index of choice is...
  short index = random(0, fileCount);

  dbgln2(F("Random file index choice is: "), index);

  // Skip all the files we don't want
  for (int i = 0; i < index; i++) {
    File entry = directory.openNextFile();
    if (!entry) {
      Serial.print(F("ERROR: Ran out of files while seeking #")); //LATER - use printf
      Serial.print(index);
      Serial.print(F(" at path: "));
      Serial.println(path); 
      assert(false); // Bail
    }
    if (entry.isDirectory()) { continue; }; // Skip subdirectories
    entry.close();
  }

  // Okay - we're up to the file we actually want info on
  File randomEntry = directory.openNextFile();
  if (!randomEntry) {
      Serial.print(F("ERROR: Couldn't open the chosen file while seeking #"));
      Serial.print(index);
      Serial.print(F(" at path: "));
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

void playSound(const __FlashStringHelper *name) {
  char buffer[MAX_PATH_LENGTH];
  if (!name) return;
  int length = strlen_P((PGM_P)name);
  if (length == 0) return;
  //if (!reserve(length)) return;
  strcpy_P(buffer, (PGM_P)name);
  playSound((const char *)&buffer[0]);
}

void playRandomSoundAtPath(const char *path) {
  char shortFileName[12];
  
  getRandomFilenameAtPath(path, &shortFileName[0]);
  
  // Concat then play
  char fullFilePath[MAX_PATH_LENGTH];
  strcpy(&fullFilePath[0], path);
  strcat(fullFilePath, shortFileName);
  dbgln2(F("Will play random sound with full path: "), fullFilePath);
  playSound(fullFilePath);
}

void playRandomSoundAtPath(const __FlashStringHelper *path) {
  char buffer[MAX_PATH_LENGTH];
  if (!path) return;
  int length = strlen_P((PGM_P)path);
  if (length == 0) return;
  //if (!reserve(length)) return;
  strcpy_P(buffer, (PGM_P)path);
  playRandomSoundAtPath((const char *)&buffer[0]);
}

void playRandomSoundForState(TurretState state) {
  
  char buffer[MAX_PATH_LENGTH];
  char *ptr = (char*)pgm_read_word(&(soundPaths[state]));
  strcpy_P(buffer, ptr);
  const char *path = &buffer[0];
  
  playRandomSoundAtPath(path);
}

void playSoundForStateWillChange(TurretState fromState, TurretState toState) {
  if (toState == tsActive) {
    playSound(FPSTR(soundFxActive));
  }
  if (fromState == tsSleeping || fromState == tsInitialising) {
    playSound(FPSTR(soundFxDeploy));
  }
}

void playSoundForStateDidChange(TurretState fromState, TurretState toState) {
  // If going to sleep from any state but autosearch,
  // play both retract and voice clip, otherwise,
  // just retract
  if (toState == tsSleeping) {
    if (fromState != tsAutoSearching) {
      playRandomSoundForState(tsSleeping);  
    }
    playSound(FPSTR(soundFxRetract));
  } else {
    playRandomSoundForState(toState); 
  }
}

// Ambient Light Level Measurement

unsigned int getAmbientLightLevel() {
  unsigned int level = analogRead(PIN_ANALOG_AMBIENT_LIGHT);
  #ifdef DEBUG
    if (ambientLightLevelOverride != MAX_UNSIGNED_INT) {
      level = ambientLightLevelOverride;
      #ifdef LOG_AMBIENT_LIGHT_LEVEL
        dbgln2(F("Overrode ambient light level with value: "), level);
      #endif
    } else {
      #ifdef LOG_AMBIENT_LIGHT_LEVEL
        dbgln2(F("Got ambient light level: "), level);
      #endif
    }
  #endif
  return level;
}

// Callback Functions

void motionWasDetected() {
  // Update the time of last motion detected
  // This routine uses interrupts so no Serial/dbg use in here!
  timeOfLastMotionDetection = millis();
}

void turretStateWillChange(TurretState toState) {
  // Update eye colour to new state colour
  eye.setColor(eyeColorForState(toState));
  // Play preparation sounds
  playSoundForStateWillChange(turretState, toState);
}

void turretStateDidChange(TurretState fromState) {
  // Update the time of last state change
  resetStateChangeTimer();
  // Play new state sound
  playSoundForStateDidChange(fromState, turretState);
  // Adjust lighting for mode
  updateLightsForStateDidChange(fromState, turretState);
}

void operationModeWillChange(OperationMode toMode) {
  // At this point, we randomise our audio responses
  randomize();
  
}

void operationModeDidChange(OperationMode fromMode) {
  updateLightsForOperationModeDidChange(fromMode, operationMode);
}

// Mode Change Functions

void updateLightsForOperationModeDidChange(OperationMode fromMode, OperationMode toMode) {
  // Trigger state update with current state - a bit yuck (clean it later)
  updateLightsForStateDidChange(turretState, turretState);
}

void updateLightsForStateDidChange(TurretState fromState, TurretState toState) {
  // If state is active and it's night time,
  // light 'em up!
  if (operationMode == omDay && lights.isOn()) {
    lights.turnOff();
  } else if (toState == tsActive && operationMode == omNight && !lights.isOn()) {
    playRandomSoundAtPath(FPSTR(soundPathDeploy));
    lights.turnOn();
  } else if (toState == tsSleeping) {
    lights.turnOff();
  }

// Causes noise due to PWM at the moment - will filter then restore
  // LATER - improve this conditional blargh
  // Light dimming - dim when searching, full bright when active
//  if (toState == tsSearching) {
//    lights.setPower(0.5);
//  } else {
//    lights.setPower(1.0);
//  }
}

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
        playSound(FPSTR(soundFxPing));
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
  
  delay(MAIN_LOOP_DELAY);
  
  #ifdef DEBUG
    if (Serial.available()) {
      char key = Serial.read();
      switch (key) {
        case 'm': 
          timeOfLastMotionDetection = millis();
          dbgln("Motion simulated");
          break;
        case 'd':
          ambientLightLevelOverride = 500;
          dbgln("Bright light simulated");
          break;
        case 'n':
          ambientLightLevelOverride = 10;
          dbgln("Low light simulated");
          break;
        case 'r':
          dbgln("Resetting...");
          audio.disable();
          Serial.flush();
          resetFunc();
          break;
        case '+':
          dbgln("Increased volume");
          audio.volume(1);
          playSound(FPSTR(soundFxPing));
          break;
        case '-':
          dbgln("Decreased volume");
          audio.volume(0);
          playSound(FPSTR(soundFxPing));
          break;
          
      }
    }
  #endif
}
