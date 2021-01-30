// Default values and balancing variables

// Ship fuel
#define STARTINGFUEL 160
#define FUELDECREMENT 1
#define FUELTIMERDELAYMS 100
#define FUELMAX 240
#define FUELTARGET 230

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
#define ROCKCOLOR makeColorRGB (150, 75, 0) //Brown
#define UNSCANNED OFF

// State and Comms Variables
enum objectTypes {ASTEROID, SHIP};
byte objectType = ASTEROID;

enum gameModes {SETUP, LOADING, INGAME, FINALE};
byte gameMode = SETUP;

enum fuelTypes {NONE, FUELA, FUELB, FUELC, FUELD, FUELE, FUELF, FUELG};
int fuelLoads[] = {40,80,120,80,120,40,80};
int fuelBurns[] = {3, 3, 3,  2, 2,  1, 1};
Color fuelColors[] = {FUELACOLOR,FUELBCOLOR,FUELCCOLOR,FUELDCOLOR,FUELECOLOR,FUELFCOLOR,FUELGCOLOR};
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
  
}

// Switch objects on long press
void checkObjectSwap () {
  if (buttonLongPressed()) {
    if (objectType == ASTEROID) {
      objectType = SHIP;
      fuel = STARTINGFUEL;
      fuelColor = STARTINGFUELCOLOR;
      fuelConsumption.set(FUELTIMERDELAYMS);
    } else {
      objectType = ASTEROID;
      makeAsteroid();
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
  if (objectType == SHIP) {
    // Sliding fuel display calculations
    int fullLEDs = round(FUELMAX / fuel); // How many LEDs are 100% full
    int fuelPerLED = FUELMAX / 6; // Amount of fuel per LED
    int remainder = fuel - (fuelPerLED * fullLEDs); // remaining fuel in partially full LED
    FOREACH_FACE(f) {
      if (f <= (fullLEDs - 1)) {
        setColorOnFace(fuelColor, f); // Make full LEDs lit
      } else if (f > (fullLEDs - 1)){
        setColorOnFace(OFF, f); // Make empty LEDs off
      } else {
        setColorOnFace(dim(fuelColor,map(remainder,0,fuelPerLED,0,255)), f); // set partial brightness to partially full LED, mapped to 255 scale
      }
    }
  } else {
    // Asteroid display stuff here
  }
}
