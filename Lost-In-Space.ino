// Default values and balancing variables

// Ship fuel
#define MAXFUEL 100
#define FUELDECREMENT 10
#define FUELTIMERDELAYMS 1000

// Scanning
#define SCANDECAYMS 1000

// Color defaults
#define FUELCOLOR ORANGE
#define O2COLOR CYAN
#define ROCKCOLOR makeColorRGB (150, 75, 0) //Brown
#define UNSCANNED OFF

// State and Comms Variables
enum objectTypes {ASTEROID, SHIP};
byte objectType = ASTEROID;

/*  COMMS SCHEME
 *   Asteroids:
 *      32         16          8        4         2         1
 *                                                        <-0->
 *   Ship(s):
 *      32         16          8        4         2         1
 *                                                        <-1->
 */

// Initialize timers
Timer fuelConsumption;

// Ship-specific variables
int fuel = MAXFUEL;

// Functions and processes that run at startup, then never again.
void setup() {
  randomize(); // Seeds randomness for any RNG use
}

// Main process loop. Each Blink runs this over and over after setup.
void loop() {

  // Check for switch object type
  checkObjectSwap();
  
  // Run functions specific to object types
  switch (objectType) {
    case ASTEROID:
      
    break;
    case SHIP:
      checkFuelConsumption();
    break;
  }
  
  // Run general functions
  commsHandler();
  displayHandler();
  
}

// Switch objects on long press
void checkObjectSwap () {
  if (buttonLongPressed()) {
    if (objectType == ASTEROID) {
      objectType = SHIP;
      fuel = MAXFUEL;
      fuelConsumption.set(FUELTIMERDELAYMS);
    } else {
      objectType = ASTEROID;
    }
  }
}

// See if fuel should be decreased or is now empty
void checkFuelConsumption () {
  if ( fuel > 0 ) {
    if (fuelConsumption.isExpired()) {
      fuel = fuel - FUELDECREMENT;
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
  
}
