#ifndef CONFIG_H
#define CONFIG_H

// To set a new nodeid edit the next line
#define NODE_ADDRESS  0x05,0x01,0x01,0x01,0x8E,0x04  // must be unique from an address space owned by you for DIY use.

// To Force Reset EEPROM to Factory Defaults set this value to 1, else 0.
#define RESET_TO_FACTORY_DEFAULTS 1

/*
There are two videos showing the ways to get hold of Node Id ranges.

Personal numbers https://www.youtube.com/watch?v=B7yfHWAk7_U 

MERG members numbers https://www.youtube.com/watch?v=gfhzXwfs-f8

*/

/*
  ======================================================================================
    End of end user configurations Changing anything below this will break the sketch.
  ======================================================================================
*/

// Choose a board, uncomment one line, see boards.h
#define ESP32_BOARD

/* Debugging -- uncomment to activate debugging statements: */
//#define DEBUG Serial

/*
  Altering the number of servos require changes made to the Boards.h for pin allocations.
*/
#define NUM_SERVOS 2
#define NUM_POS    3  
#define NUM_EVENT  56  

// Board definitions
#define MANU " OpenLCB "              // The manufacturer of node
#define MODEL BOARD " 2Servo6in6out "        // The default model of the board
#define HWVERSION " ESP 1 Basic "     // Hardware version
#define SWVERSION " 1.0.8 "           // Software version

// Global defs
const bool USE_90_ON_STARTUP = true;  

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif // CONFIG_H
