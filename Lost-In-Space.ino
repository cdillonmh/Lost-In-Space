// Default values and balancing variables

// Ship fuel
#define STARTINGFUEL 160
#define FUELDECREMENT 1
#define FUELTIMERDELAYMS 100
#define FUELMAX 240
#define FUELTARGET 230
#define FUELRARITY 21 // Larger is more rare

// Gameplay Rates
#define SCANDECAYMS 5000
#define GATHERTIMEMS 1500

// Color defaults
#define STARTINGFUELCOLOR WHITE
#define FUELACOLOR RED
#define FUELBCOLOR makeColorRGB (128, 0, 128) //Purple
#define FUELCCOLOR GREEN
#define FUELDCOLOR YELLOW
#define FUELECOLOR CYAN
#define FUELFCOLOR BLUE
#define FUELGCOLOR MAGENTA
#define ROCKCOLOR OFF //makeColorRGB (150, 75, 0) //Brown
#define UNSCANNED OFF //makeColorRGB (20, 10, 0) //Dark Brown?

// State and Comms Variables
enum objectTypes {ASTEROID, SHIP};
byte objectType = ASTEROID;

enum gameModes {SETUP, LOADING, INGAME, FINALE};
byte gameMode = SETUP;

enum fuelTypes {NONE, FUELA, FUELB, FUELC, FUELD, FUELE, FUELF, FUELG};
Color fuelColors[] = {ROCKCOLOR,FUELACOLOR,FUELBCOLOR,FUELCCOLOR,FUELDCOLOR,FUELECOLOR,FUELFCOLOR,FUELGCOLOR};
int fuelLoads[] = {40,80,120,80,120,40,80};
int fuelBurns[] = {3, 3, 3,  2, 2,  1, 1};
int fuelIndexShift = 0;

/*  IN-GAME COMMS SCHEME
 *   Asteroids:
 *      32         16          8         4         2         1
 *     <-Fuel Type on this face?->      <-Game Mode->      <-0->
 *   Ship(s):
 *      32         16          8         4         2         1
 *                        <-Gather?->   <-Game Mode->      <-1->
 */

// Ship-specific variables
Timer fuelConsumption;
int fuel = STARTINGFUEL;
int burnRate = FUELDECREMENT;
byte fuelType = NONE;
Color fuelColor = STARTINGFUELCOLOR;

// Asteroid-specific variables
Timer scanDecay;
bool isScanned = false;
bool scanFading = true;
bool freshScan = false;
int fuelContents[] = {NONE, NONE, NONE, NONE, NONE, NONE};

// Functions and processes that run at startup, then never again.
void setup() {
  randomize(); // Seeds randomness for any RNG use
  makeAsteroid(); // Setup and seed this asteroid
}

// Main process loop. Each Blink runs this over and over after setup.
void loop() {

  // Check for switch object type
  checkObjectSwap();
  
  // Run functions specific to object types
  switch (objectType) {
    case ASTEROID:
      checkScan();
    break;
    case SHIP:
      checkFuelConsumption();
    break;
  }
  
  // Run general functions
  commsHandler();
  displayHandler();
  
}

// Creating this object as an asteroid
void makeAsteroid () {
  seedFuel();
}

// Randomize and distribute fuel
void seedFuel () {
  FOREACH_FACE(f) {
    int randomFuel = random(FUELRARITY) - (FUELRARITY-7);
    if (randomFuel < 0) {
      randomFuel = 0;
    }
    fuelContents[f] = randomFuel;
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
    } else {
      objectType = ASTEROID;
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
// TODO: Replace temp isAlone with actual check for ship
bool shipConnected () {
  if (!isAlone()) {
    return true;
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
    } else {
      destroyShip();
    }
  }
}

// Out of fuel, game over.
void destroyShip () {
  
}

// Handle parsing bitwise communications
void commsHandler () {
  
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
    if (!isScanned) {
      FOREACH_FACE (f) {
        setColorOnFace(UNSCANNED, f);
      }
    } else if (!scanFading) {
      FOREACH_FACE (f) {
        setColorOnFace(fuelColors[fuelContents[f]],f);
      }
    } else {
      int scanRemaining = map(scanDecay.getRemaining(),0,SCANDECAYMS,0,255);
      FOREACH_FACE (f) {
      setColorOnFace(dim(fuelColors[fuelContents[f]],scanRemaining),f);
      }
    }
  }
}
