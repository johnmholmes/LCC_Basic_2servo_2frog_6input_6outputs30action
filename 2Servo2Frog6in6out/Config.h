#ifndef CONFIG_H
#define CONFIG_H

// To set a new nodeid edit the next line
#define NODE_ADDRESS  0x05,0x01,0x01,0x01,0x8E,0x04  // must be unique from an address space owned by you or DIY

// To Force Reset EEPROM to Factory Defaults set this value to 1, else 0.
#define RESET_TO_FACTORY_DEFAULTS 1

// Choose a board, uncomment one line, see boards.h
#define ESP32_BOARD

/* Debugging -- uncomment to activate debugging statements: */
//#define DEBUG Serial

/*
  Altering the number of servos require changes made to the Boards.h for pin allocations.
*/
#define NUM_SERVOS 2
#define NUM_POS    3  
#define NUM_EVENT  26  // 2servo * 3 positions + 4 events per servo * 2 + 12 input

// Board definitions
#define MANU " OpenLCB "              // The manufacturer of node
#define MODEL BOARD " 2Servo6in6out "        // The default model of the board
#define HWVERSION " ESP 1 Basic "     // Hardware version
#define SWVERSION " 1.0.4 "           // Software version

// Global defs
const bool USE_90_ON_STARTUP = true;  

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif // CONFIG_H
