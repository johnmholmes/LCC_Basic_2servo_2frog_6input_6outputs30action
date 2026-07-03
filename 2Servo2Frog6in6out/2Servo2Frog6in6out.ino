#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board definitions, see "Boards.h"
#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.
#include <ESP32Servo.h>

#define OLCB_NO_BLUE_GOLD // Do not delete

// Define Frog Relay Output Pins
#define FROG_PIN_0  25  // Frog relay for Servo 0 (Servo Pin 32)
#define FROG_PIN_1  26  // Frog relay for Servo 1 (Servo Pin 33)

// Define Discrete Pull-up Input Pins (6 additional inputs)
#define NUM_INPUTS 6
const uint8_t inputPins[NUM_INPUTS] = { 16, 17, 5, 18, 19, 21 }; 

extern "C" {
    #define N(x) xN(x)     
    #define xN(x) #x       
const char configDefInfo[] PROGMEM =
    CDIheader R"(
    <name>Application Configuration</name>
    <hints><visibility hideable='yes' hidden='yes' ></visibility></hints>
    <group>
        <name>Turnout Servo Speed Configuration</name>
         <description>Ensure Servos are powered from a separate 5 volt power supply. Not from the shield</description>
        <int size='1'>
          <name>Speed 5-50 (delay between steps)</name>
          <min>5</min><max>50</max>
          <hints><slider tickSpacing='15' immediate='yes' showValue='yes'> </slider></hints>
        </int>
    </group>
    <group replication=')" N(NUM_SERVOS) R"('>
        <name>Servos</name>
        <repname>Servo Pin 32</repname>
        <repname>Servo Pin 33</repname>
        <string size='24'><name>Servo Location On Layout.</name></string>

        <group replication=')" N(NUM_POS) R"('>
        <name>  Closed     Midpoint     Thrown</name>
            <repname>Position</repname>
            <eventid><name>EventID</name></eventid>
            <int size='1'>
                <name>Servo Position in approximate degrees range 0 to 180. Take care when using the slider small changes are best done using the text box</name>
                <min>0</min><max>180</max>
                <hints><slider tickSpacing='45' immediate='yes' showValue='yes'> </slider></hints>
            </int>
        </group>
        
        <eventid><name>Servo Reached Closed (Pos 1) Event</name></eventid>
        <eventid><name>Servo Reached Thrown (Pos 3) Event</name></eventid>
        <eventid><name>Servo Passed Midpoint Moving to Thrown (Frog Relay)</name></eventid>
        <eventid><name>Servo Passed Midpoint Moving to Closed (Frog Relay)</name></eventid>
    </group>
    <group replication=')" N(NUM_INPUTS) R"('>
        <name>Inputs</name>
        <repname>Input Pin D16</repname>
        <repname>Input Pin D17</repname>
        <repname>Input Pin D5</repname>
        <repname>Input Pin D18</repname>
        <repname>Input Pin D19</repname>
        <repname>Input Pin D21</repname>
        <string size='24'><name>Input Description / Location</name></string>
        <eventid><name>Input HIGH State Event</name></eventid>
        <eventid><name>Input LOW State Event</name></eventid>
    </group>
    )" CDIfooter;
} 

typedef struct {
      EVENT_SPACE_HEADER eventSpaceHeader; 
      char nodeName[20];  
      char nodeDesc[24];  
      uint8_t servodelay; 
      
      struct {
        char desc[24];        
        
        struct {
          EventID eid;       
          uint8_t angle;     
        } pos[NUM_POS];

        EventID reachedClosedEid; 
        EventID reachedThrownEid; 
        EventID passedMidThrownEid; 
        EventID passedMidClosedEid; 
        
      } servos[NUM_SERVOS];

      // Input structures mapped directly following the Servos segment
      struct {
        char desc[24]; // Added description variable text block here
        EventID highStateEid;
        EventID lowStateEid;
      } inputs[NUM_INPUTS];

  uint8_t curpos[NUM_SERVOS]; 
} MemStruct;                

// --- GLOBAL VARIABLE FOR TARGET tracking (Kept in volatile RAM only now) ---
uint8_t curpos[NUM_SERVOS]; 

// Dynamic tracker to ensure feedback events only fire once per action
bool servoMoving[NUM_SERVOS] = {false, false};
bool midCrossed[NUM_SERVOS] = {false, false}; 

// Track physical inputs
bool lastInputState[NUM_INPUTS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};

// Array to easily map hardware frog pins to loop index tracking
const uint8_t frogPins[NUM_SERVOS] = { FROG_PIN_0, FROG_PIN_1 };

extern "C" {
    // Total events: 14 (2 Servos * 7) + 12 (6 Inputs * 2) = 26
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        // ================= SERVO 0 (7 Events) =================
        CEID(servos[0].pos[0].eid),          
        CEID(servos[0].pos[1].eid),          
        CEID(servos[0].pos[2].eid),          
        PEID(servos[0].reachedClosedEid),    
        PEID(servos[0].reachedThrownEid),    
        PEID(servos[0].passedMidThrownEid),  
        PEID(servos[0].passedMidClosedEid),  

        // ================= SERVO 1 (7 Events) =================
        CEID(servos[1].pos[0].eid),          
        CEID(servos[1].pos[1].eid),          
        CEID(servos[1].pos[2].eid),          
        PEID(servos[1].reachedClosedEid),    
        PEID(servos[1].reachedThrownEid),    
        PEID(servos[1].passedMidThrownEid),  
        PEID(servos[1].passedMidClosedEid),

        // ================= INPUTS 0-5 (12 Events) =================
        PEID(inputs[0].highStateEid), PEID(inputs[0].lowStateEid),
        PEID(inputs[1].highStateEid), PEID(inputs[1].lowStateEid),
        PEID(inputs[2].highStateEid), PEID(inputs[2].lowStateEid),
        PEID(inputs[3].highStateEid), PEID(inputs[3].lowStateEid),
        PEID(inputs[4].highStateEid), PEID(inputs[4].lowStateEid),
        PEID(inputs[5].highStateEid), PEID(inputs[5].lowStateEid)
    };

    extern const char SNII_const_data[] PROGMEM = 
    "\001" MANU "\000" MODEL "\000" HWVERSION "\000" SWVERSION " " OlcbCommonVersion;
}

uint8_t protocolIdentValue[6] = {   
        pSimple | pDatagram | pMemConfig | pPCEvents | !pIdent    | pTeach     | !pStream   | !pReservation, 
        pACDI   | pSNIP     | pCDI       | !pRemote  | !pDisplay  | !pTraction | !pFunction | !pDCC        , 
        0, 0, 0, 0                                                                                             
};

Servo servo[NUM_SERVOS];
uint8_t servoActual[NUM_SERVOS];
uint8_t servoTarget[NUM_SERVOS];
uint8_t servopin[]  = { SERVOPINS };

#define SERVO_DELAY_OFFSET  EEADDR(servodelay)
bool posdirty = false;

void servoSet(); 

void reportConfig() {
  dP("\n 2Servo Turnout with Frog Midpoint Events and 6 Discrete Inputs");
  dP("\nFile: " __FILE__);
  dP("\nUsing " BOARD);
  dP("\nNode ID="); dP(TOSTRING((NODE_ADDRESS)));
  dP("\nServo pins:"); for(int i=0; i<2; i++) { dP(" "); dP(servopin[i]); }
  dP("\nDiscrete Input Pins:"); for(int i=0; i<NUM_INPUTS; i++) { dP(" "); dP(inputPins[i]); }
  dP("\nCAN pins: Tx="); dP(CAN_TX_PIN); dP(" RX="); dP(CAN_RX_PIN);
}

void userInitAll()
{ 
  NODECONFIG.put(EEADDR(nodeName), ESTRING("Esp32"));
  NODECONFIG.put(EEADDR(nodeDesc), ESTRING("2Servos6Inputs"));
  NODECONFIG.update(SERVO_DELAY_OFFSET, 20);
  
  // Clear servo descriptions and angles
  for(uint8_t i = 0; i < NUM_SERVOS; i++) {
    NODECONFIG.put(EEADDR(servos[i].desc), ESTRING(""));
    for(int p=0; p<NUM_POS; p++) {
      NODECONFIG.update(EEADDR(servos[i].pos[p].angle), 90);
    }
  }

  // Clear discrete input description strings in EEPROM
  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    NODECONFIG.put(EEADDR(inputs[i].desc), ESTRING(""));
  }
  
  EEPROMcommit;
}

enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
    // Check if the event index belongs to the Servos
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7; 
        int localIndex = index % 7;

        if (localIndex < 3) {
            if (curpos[ch] == localIndex) return VALID;
            else return INVALID;
        }
        
        if (!servoMoving[ch] && servoActual[ch] == servoTarget[ch]) {
            if (localIndex == 3 && curpos[ch] == 0) return VALID; 
            if (localIndex == 4 && curpos[ch] == 2) return VALID; 
        }
        return INVALID;
    } 
    // Handle state evaluation for the 6 additional digital inputs
    else if (index < NUM_EVENT) {
        int inputIdx = (index - (NUM_SERVOS * 7)) / 2;
        int stateType = (index - (NUM_SERVOS * 7)) % 2; // 0 = HIGH event, 1 = LOW event
        
        bool reading = digitalRead(inputPins[inputIdx]);
        if (stateType == 0 && reading == HIGH) return VALID;
        if (stateType == 1 && reading == LOW) return VALID;
        return INVALID;
    }
    
    return INVALID;
}  

void pceCallback(uint16_t index) {
    dP("\npceCallback, index="); dP((uint16_t)index);
    
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7;
        int localIndex = index % 7;

        if (ch < NUM_SERVOS && localIndex < 3) {
            curpos[ch] = localIndex;
            servoTarget[ch] = NODECONFIG.read( EEADDR(servos[ch].pos[localIndex].angle) );
            servoMoving[ch] = true; 
            midCrossed[ch] = false; 
            
            dP("\n servo#"); dP(ch); dP(" position#"); dP(localIndex); dP(" target angle="); dP(servoTarget[ch]); 
        }
    }
}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);  
#include "OpenLCBMid.h"    

#ifndef P
  #define P(...) Serial.print( __VA_ARGS__)
#endif
#ifndef PVL
  #define PVL(x) { P("\n"); P(#x  "="); P(x); }
#endif

void printMem() {
  PVL(NODECONFIG.read(SERVO_DELAY_OFFSET));
  for(int s=0;s<NUM_SERVOS;s++) {
    P("\nServo "); P(s); 
    uint8_t cp = curpos[s]; 
    P(" cp="); P(cp);
    uint8_t angle1 = NODECONFIG.read(EEADDR(servos[s].pos[0].angle)); 
    P(" angle1="); P(angle1);
    uint8_t angle2 = NODECONFIG.read(EEADDR(servos[s].pos[1].angle));
    P(" angle2="); P(angle2);
    uint8_t angle3 = NODECONFIG.read(EEADDR(servos[s].pos[2].angle)); 
    P(" angle3="); P(angle3);
  }
}

void userConfigWritten(uint32_t address, uint16_t length, uint16_t func)
{
  EEPROMcommit;
  servoSet();
}

void servoBackgroundTask(void * parameter) {
  for(;;) {
    uint8_t sliderVal = NODECONFIG.read( SERVO_DELAY_OFFSET );
    if (sliderVal < 1) sliderVal = 1; 

    uint8_t stepSize = sliderVal / 5; 
    if (stepSize < 1) stepSize = 1;

    vTaskDelay(pdMS_TO_TICKS(20));

    static long lastmove = 0;
    
    for(int i=0; i<NUM_SERVOS; i++) {
      uint8_t midAngle = NODECONFIG.read( EEADDR(servos[i].pos[1].angle) );
      uint8_t oldActual = servoActual[i]; 

      if(servoTarget[i] == servoActual[i] ) {
        if (servoMoving[i]) {
          servoMoving[i] = false; 
          uint16_t servoBaseIndex = i * 7; 

          if (curpos[i] == 0) {
            OpenLcb.produce(servoBaseIndex + 3); 
            dP("\n Feedback: Servo #"); dP(i); dP(" reached CLOSED.");
          } 
          else if (curpos[i] == 2) {
            OpenLcb.produce(servoBaseIndex + 4); 
            dP("\n Feedback: Servo #"); dP(i); dP(" reached THROWN.");
          }
        }
        continue;
      }
      
      if(servoTarget[i] > servoActual[i]) {
        if ((servoTarget[i] - servoActual[i]) > stepSize) {
          servoActual[i] += stepSize;
        } else {
          servoActual[i] = servoTarget[i]; 
        }
      }
      else if(servoTarget[i] < servoActual[i]) {
        if ((servoActual[i] - servoTarget[i]) > stepSize) {
          servoActual[i] -= stepSize;
        } else {
          servoActual[i] = servoTarget[i]; 
        }
      }

      if (servoMoving[i] && !midCrossed[i]) {
        if (curpos[i] == 2) { 
          if ((oldActual < midAngle && servoActual[i] >= midAngle) || 
              (oldActual > midAngle && servoActual[i] <= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], HIGH); 
             uint16_t targetIndex = (i * 7) + 5; 
             OpenLcb.produce(targetIndex);
          }
        }
        else if (curpos[i] == 0) {
          if ((oldActual > midAngle && servoActual[i] <= midAngle) || 
              (oldActual < midAngle && servoActual[i] >= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], LOW);  
             uint16_t targetIndex = (i * 7) + 6; 
             OpenLcb.produce(targetIndex);
          }
        }
      }
      
      if(!servo[i].attached()) { 
        servo[i].attach(servopin[i]);
        vTaskDelay(pdMS_TO_TICKS(50));
      }
      servo[i].write(servoActual[i]);
      lastmove = millis();
      posdirty = true;
    }

    if( lastmove && (millis()-lastmove)>4000) {
      for(int i=0; i<NUM_SERVOS; i++) servo[i].detach();
      lastmove = 0;
    }
  }
}

// Background task checking hardware pullups & producing layout events
void inputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(30)); // 30ms polling interval behaves safely as a software debounce
    
    for(int i = 0; i < NUM_INPUTS; i++) {
      bool currentState = digitalRead(inputPins[i]);
      
      if(currentState != lastInputState[i]) {
        lastInputState[i] = currentState;
        
        // Calculate where this dynamic index begins in the OpenLCB pool
        uint16_t inputBaseIndex = (NUM_SERVOS * 7) + (i * 2);
        
        if(currentState == HIGH) {
          OpenLcb.produce(inputBaseIndex); // Index for HIGH State Event
          dP("\n Input #"); dP(i); dP(" changed to HIGH.");
        } else {
          OpenLcb.produce(inputBaseIndex + 1); // Index for LOW State Event
          dP("\n Input #"); dP(i); dP(" changed to LOW.");
        }
      }
    }
  }
}

void servoStartUp() {
  for(int i=0; i<NUM_SERVOS; i++) {
    curpos[i] = 0; 
    digitalWrite(frogPins[i], LOW);
    
    if( USE_90_ON_STARTUP ) servoActual[i] = 90;
    else servoActual[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
    
    servo[i].attach(servopin[i]);
    servo[i].write(servoActual[i]);
    servoMoving[i] = false; 
    midCrossed[i] = false;
    delay(100);
  }
  servoSet();
}

void servoSet() {
  for(int i=0; i<NUM_SERVOS; i++) {
    servoTarget[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
  }
}

void setup()
{
  Serial.begin(115200); while(!Serial);
  delay(2000);
  dP("\n Setup Initialized");

  // Configure Frog Relay Pins as Outputs
  pinMode(FROG_PIN_0, OUTPUT);
  pinMode(FROG_PIN_1, OUTPUT);

  // Initialize the 6 hardware inputs with custom internal Pullups
  for(int i = 0; i < NUM_INPUTS; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
    lastInputState[i] = digitalRead(inputPins[i]); // Read active level at start up
  }

  EEPROMbegin;
  NodeID nodeid(NODE_ADDRESS);      
  Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
  reportConfig();

  servoStartUp();

  // Task for moving the turnouts
  xTaskCreatePinnedToCore(
    servoBackgroundTask,   
    "ServoTask",           
    4096,                  
    NULL,                  
    1,                     
    NULL,                  
    0                      
  );

  // Dedicated Core 0 Task to monitor physical logic switches
  xTaskCreatePinnedToCore(
    inputBackgroundTask,   
    "InputTask",           
    3072,                  
    NULL,                  
    1,                     
    NULL,                  
    0                      
  );
}

void loop() {
  Olcb_process();        
}
