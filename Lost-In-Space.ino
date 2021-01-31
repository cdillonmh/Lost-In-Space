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
#define ENERGYBCOLOR makeColorRGB (128, 0, 128) //Purple
#define ENERGYCCOLOR GREEN
#define ENERGYDCOLOR YELLOW
#define ENERGYECOLOR CYAN
#define ENERGYFCOLOR BLUE // not needed anymore
#define ENERGYGCOLOR MAGENTA // not needed anymore
#define PARTICLECOLOR OFF //makeColorRGB (150, 75, 0) //Brown
#define UNSCANNED OFF //makeColorRGB (20, 10, 0) //Dark Brown?

// State and Comms Variables
enum objectTypes {PARTICLE, SHIP};
byte objectType = PARTICLE;

enum gameModes {SETUP, LOADING, INGAME, FINALE};
byte gameMode = SETUP;

enum energyTypes {NONE, ENERGYA, ENERGYB, ENERGYC, ENERGYD, ENERGYE};
Color energyColor[] = {PARTICLECOLOR,ENERGYACOLOR,ENERGYBCOLOR,ENERGYCCOLOR,ENERGYDCOLOR,ENERGYECOLOR};
int energyLoads[] = {60,240,150,60,150};
int energyDecay[] = {4,4,2,1,1};
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
Timer energyDecay;
int energy = STARTINGENERGY;
int decayRate = ENERGYDECREMENT;
byte energyType = NONE;
Color energyColor = STARTINGENERGYCOLOR;
bool receivedOnFace[] = {false,false,false,false,false,false};

// Particle-specific variables
Timer scanDecay;
Timer gatherTimer;
bool isScanned = false;
bool scanFading = true;
bool freshScan = false;
int fuelContents[] = {NONE, NONE, NONE, NONE, NONE, NONE};
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
    int randomEnergy = random(EnergyRARITY) - (EnergyRARITY-5);
    if (randomEnergy < 0) {
      randomEnergy = 0;
    }
    energyContents[f] = randomEnergy;
  }
}

// Switch objects on long press
void checkObjectSwap () {
  if (buttonLongPressed()) {
    if (objectType == ASTEROID) {
      objectType = SHIP;
      fuel = STARTINGFUEL;
      fuelColor = STARTINGFUELCOLOR;
      fuelConsumption.set(FUELTIMERDELAYMS);
      burnRate = FUELDECREMENT;
      fuelIndexShift = random(6);
    } else {
      makeAsteroid();
    }
  }
}

// Handle scan display
void checkScan () {
  if (shipConnected()) {
    if (!isScanned){
      seedFuel();
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

// Return true if asteroid is connected
bool asteroidConnected () {
  bool asteroidFound = false; 
  FOREACH_FACE(f) {
    if (isAsteroidOnFace(f)) {
      asteroidFound = true;
    }
  }
  return asteroidFound;
}

void checkFuelSend () {
  FOREACH_FACE(f) {
    if (gatherTimer.isExpired() && sendingOnFace[f]) {
        sendingOnFace[f] = false;
        fuelContents[f] = NONE;
    }
  }
  if (shipConnected() && buttonPressed()) {
    FOREACH_FACE(f) {
      if (fuelContents[f] != NONE && isShipOnFace(f)) {
        sendingOnFace[f] = true;
        gatherTimer.set(GATHERTIMEMS);
      }
    }
  }
  if (!shipConnected()) {
    FOREACH_FACE(f) {
      sendingOnFace[f] = false;
    }
  }
}

void checkFuelReceive() {
  FOREACH_FACE(f) {
    if (isAsteroidOnFace(f) && sendingFuelOnFace(f) && !receivedOnFace[f]) {
      int receivedFuelType = getFuelContents(getLastValueReceivedOnFace(f));
      // Set fuel color to new fuel color
      fuelColor = fuelColors[receivedFuelType];
      // Determine actual shifted index
      receivedFuelType = ((receivedFuelType - 1) + fuelIndexShift) % 6;
      // Add fuel load
      fuel = fuel + fuelLoads[receivedFuelType];
      // Set burn rate
      burnRate = fuelBurns[receivedFuelType];
      // Mark received on face
      receivedOnFace[f] = true;
    }
  }
  if (!asteroidConnected()) {
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

// Check face for asteroid
bool isAsteroidOnFace (int f) {
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

bool sendingFuelOnFace (int f) {
  if (!isValueReceivedOnFaceExpired(f)){
    if (getFuelContents(getLastValueReceivedOnFace(f)) != NONE) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

// See if fuel should be decreased or is now empty
void checkFuelConsumption () {
  if ( fuel > 0 ) {
    if (fuelConsumption.isExpired()) {
      fuel = fuel - burnRate;
      fuelConsumption.set(FUELTIMERDELAYMS);
    }
  } else {
    fuel = 0;
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
        setValueSentOnFace((fuelContents[f] << 3) + (gameMode << 1) + 0, f);
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

byte getFuelContents (byte data) {
  return ((data >> 3) & 7);
}

// Handle LED display
void displayHandler () {
  if (objectType == SHIP) { // Ship display handler
    // Sliding fuel display calculations
    int fuelPerLED = round(FUELMAX / 6); // Amount of fuel per LED
    int fullLEDs = round(fuel / fuelPerLED); // How many LEDs are 100% full
    int remainder = round(fuel % fuelPerLED); // remaining fuel in partially full LED
    FOREACH_FACE(f) {
      if (f < (fullLEDs)) {
        setColorOnFace(fuelColor, f); // Make full LEDs lit
      } else if (f > (fullLEDs)){
        setColorOnFace(OFF, f); // Make empty LEDs off
      } else {
        setColorOnFace(dim(fuelColor,map(remainder,0,fuelPerLED,0,255)), f); // set partial brightness to partially full LED, mapped to 255 scale
      }
    }
  } else { // Asteroid display handler
    if (!isScanned) { // Unscanned asteroids display as blank (for now)
      FOREACH_FACE (f) {
        setColorOnFace(UNSCANNED, f);
      }
    } else if (!scanFading) { // Scanned but not fading (usually because ship still connected) displays as full brightness
      FOREACH_FACE (f) {
        setColorOnFace(fuelColors[fuelContents[f]],f);
      }
    } else { // Scanned and fading, dim to match fading scan
      int scanRemaining = map(scanDecay.getRemaining(),0,SCANDECAYMS,0,255);
      FOREACH_FACE (f) {
      setColorOnFace(dim(fuelColors[fuelContents[f]],scanRemaining),f);
      }
    }
  }
}
