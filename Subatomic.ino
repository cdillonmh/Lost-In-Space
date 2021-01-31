// Default values and balancing variables

// Ship charge
#define STARTINGENERGY 240
#define ENERGYDECREMENT 1
#define ENERGYTIMERDELAYMS 100
#define ENERGYMAX 360
#define ENERGYTARGET 230
#define ENERGYRARITY 15 // Larger is more rare

// Gameplay Rates
#define SCANDECAYMS 5000
#define ABSORBTIMEMS 100

// Color defaults
#define STARTINGENERGYCOLOR WHITE
#define ENERGYACOLOR RED
#define ENERGYBCOLOR YELLOW
#define ENERGYCCOLOR GREEN
#define ENERGYDCOLOR MAGENTA
#define ENERGYECOLOR BLUE
//#define ENERGYFCOLOR BLUE // not needed anymore
//#define ENERGYGCOLOR MAGENTA // not needed anymore
#define PARTICLECOLOR OFF //makeColorRGB (150, 75, 0) //Brown
#define UNSCANNED OFF //makeColorRGB (20, 10, 0) //Dark Brown?

// State and Comms Variables
enum objectTypes {PARTICLE, SHIP};
byte objectType = PARTICLE;

enum gameModes {SETUP, LOADING, INGAME, FINALE};
byte gameMode = SETUP;

enum energyTypes {NONE, ENERGYA, ENERGYB, ENERGYC, ENERGYD, ENERGYE};
Color energyColors[] = {PARTICLECOLOR,ENERGYACOLOR,ENERGYBCOLOR,ENERGYCCOLOR,ENERGYDCOLOR,ENERGYECOLOR};
int energyLoads[] = {60,240,150,60,150};
int energyDecays[] = {4,4,2,1,1};
int energyIndexShift = 0;

/*  IN-GAME COMMS SCHEME
 *   Particles:
 *      32         16          8         4         2         1
 *     <-Energy Type on this face?->      <-Game Mode->      <-0->
 *   Ship(s):
 *      32         16          8         4         2         1
 *                                      <-Game Mode->      <-1->
 */

// Ship-specific variables
Timer energyDecayRate;
int energy = STARTINGENERGY;
int decayRate = ENERGYDECREMENT;
byte energyType = NONE;
Color energyColor = STARTINGENERGYCOLOR;
bool receivedOnFace[] = {false,false,false,false,false,false};

// Particle-specific variables
Timer scanDecay;
Timer absorbTimer;
bool isScanned = false;
bool scanFading = true;
bool freshScan = false;
int energyContents[] = {NONE, NONE, NONE, NONE, NONE, NONE};
bool sendingOnFace[] = {false,false,false,false,false,false};

// Functions and processes that run at startup, then never again.
void setup() {
  randomize(); // Seeds randomness for any RNG use
  makeParticle(); // Setup and seed this particle
}

// Main process loop. Each Blink runs this over and over after setup.
void loop() {

  // Check for switch object type
  checkObjectSwap();
  
  // Run functions specific to object types
  switch (objectType) {
    case PARTICLE:
      checkScan();
      checkEnergySend();
    break;
    case SHIP:
      checkEnergyDecay();
      checkEnergyReceive();
    break;
  }
  
  // Run general functions
  commsHandler();
  displayHandler();
  
}

// Creating this object as a particle
void makeParticle () {
  objectType = PARTICLE;
  seedEnergy();
}

// Randomize and distribute energy
void seedEnergy () {
  FOREACH_FACE(f) {
    int randomEnergy = random(ENERGYRARITY) - (ENERGYRARITY-5);
    if (randomEnergy < 0) {
      randomEnergy = 0;
    }
    energyContents[f] = randomEnergy;
  }
}

// Switch objects on long press
void checkObjectSwap () {
  if (buttonLongPressed()) {
    if (objectType == PARTICLE) {
      objectType = SHIP;
      energy = STARTINGENERGY;
      energyColor = STARTINGENERGYCOLOR;
      energyDecayRate.set(ENERGYTIMERDELAYMS);
      decayRate = ENERGYDECREMENT;
      energyIndexShift = random(4);
    } else {
      makeParticle();
    }
  }
}

// Handle scan display
void checkScan () {
  if (shipConnected()) {
    if (!isScanned){
      seedEnergy();
    }
    scanFading = false;
    isScanned = true;
    freshScan = true;
  } else {
    if (freshScan == true) {
      freshScan = false;
      scanFading = true;
      scanDecay.set(SCANDECAYMS);
    } else {
      if (scanFading == true && scanDecay.isExpired()) {
        scanFading = false;
        isScanned = false;
      }
    }
  }
}

// Return true if ship is connected
bool shipConnected () {
  bool shipFound = false; 
  FOREACH_FACE(f) {
    if (isShipOnFace(f)) {
      shipFound = true;
    }
  }
  return shipFound;
}

// Return true if particle is connected
bool particleConnected () {
  bool particleFound = false; 
  FOREACH_FACE(f) {
    if (isParticleOnFace(f)) {
      particleFound = true;
    }
  }
  return particleFound;
}

void checkEnergySend () {
  FOREACH_FACE(f) {
    if (absorbTimer.isExpired() && sendingOnFace[f]) {
        sendingOnFace[f] = false;
        energyContents[f] = NONE;
    }
  }
  if (shipConnected() && buttonPressed()) {
    FOREACH_FACE(f) {
      if (energyContents[f] != NONE && isShipOnFace(f)) {
        sendingOnFace[f] = true;
        absorbTimer.set(ABSORBTIMEMS);
      }
    }
  }
  if (!shipConnected()) {
    FOREACH_FACE(f) {
      sendingOnFace[f] = false;
    }
  }
}

void checkEnergyReceive() {
  FOREACH_FACE(f) {
    if (isParticleOnFace(f) && sendingEnergyOnFace(f) && !receivedOnFace[f]) {
      int receivedEnergyType = getEnergyContents(getLastValueReceivedOnFace(f));
      // Set energy color to new energy color
      energyColor = energyColors[receivedEnergyType];
      // Determine actual shifted index
      receivedEnergyType = ((receivedEnergyType - 1) + energyIndexShift) % 5;
      // Add energy load
      energy = energy + energyLoads[receivedEnergyType];
      // Set decay rate
      decayRate = energyDecays[receivedEnergyType];
      // Mark received on face
      receivedOnFace[f] = true;
    }
  }
  if (!particleConnected()) {
    FOREACH_FACE(f) {
      receivedOnFace[f] = false;
    }
  }
}

// Check face for ship
bool isShipOnFace (int f) {
  if (!isValueReceivedOnFaceExpired(f)){
    if (getObjectType(getLastValueReceivedOnFace(f)) == 1) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

// Check face for particle
bool isParticleOnFace (int f) {
  if (!isValueReceivedOnFaceExpired(f)){
    if (getObjectType(getLastValueReceivedOnFace(f)) == 0) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool sendingEnergyOnFace (int f) {
  if (!isValueReceivedOnFaceExpired(f)){
    if (getEnergyContents(getLastValueReceivedOnFace(f)) != NONE) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

// See if energy should be decreased or is now empty
void checkEnergyDecay () {
  if ( energy > 0 ) {
    if (energyDecayRate.isExpired()) {
      energy = energy - decayRate;
      energyDecayRate.set(ENERGYTIMERDELAYMS);
    }
  } else {
    energy = 0;
    destroyShip();
  }
}

// Out of fuel, game over.
void destroyShip () {
}

// Handle outgoing communications
void commsHandler () {
  if (objectType == SHIP) {
    setValueSentOnAllFaces((gameMode << 1) + 1);
  } else {
    FOREACH_FACE(f) {
      if (sendingOnFace[f]) {
        setValueSentOnFace((energyContents[f] << 3) + (gameMode << 1) + 0, f);
      } else {
        setValueSentOnFace((NONE << 3) + (gameMode << 1) + 0, f);
      }
    }
  }
}

byte getObjectType (byte data) {
  return (data & 1);
}

byte getGameMode (byte data) {
  return ((data >> 1) & 3);
}

byte getEnergyContents (byte data) {
  return ((data >> 3) & 7);
}

// Handle LED display
void displayHandler () {
  if (objectType == SHIP) { // Ship display handler
    // Sliding energy display calculations
    int energyPerLED = round(ENERGYMAX / 6); // Amount of energy per LED
    int fullLEDs = round(energy / energyPerLED); // How many LEDs are 100% full
    int remainder = round(energy % energyPerLED); // remaining energy in partially full LED
    FOREACH_FACE(f) {
      if (f < (fullLEDs)) {
        setColorOnFace(energyColor, f); // Make full LEDs lit
      } else if (f > (fullLEDs)){
        setColorOnFace(OFF, f); // Make empty LEDs off
      } else {
        setColorOnFace(dim(energyColor,map(remainder,0,energyPerLED,0,255)), f); // set partial brightness to partially full LED, mapped to 255 scale
      }
    }
  } else { // Particle display handler
    if (!isScanned) { // Unscanned particles display as blank (for now)
      FOREACH_FACE (f) {
        setColorOnFace(UNSCANNED, f);
      }
    } else if (!scanFading) { // Scanned but not fading (usually because ship still connected) displays as full brightness
      FOREACH_FACE (f) {
        setColorOnFace(energyColors[energyContents[f]],f);
      }
    } else { // Scanned and fading, dim to match fading scan
      int scanRemaining = map(scanDecay.getRemaining(),0,SCANDECAYMS,0,255);
      FOREACH_FACE (f) {
      setColorOnFace(dim(energyColors[energyContents[f]],scanRemaining),f);
      }
    }
  }
}
