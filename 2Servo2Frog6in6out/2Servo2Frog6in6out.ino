#include "Config.h"   // Contains configuration, see "Config.h"
#include "Boards.h"   // Contains Board definitions, see "Boards.h"
#include "mdebugging.h"           // debugging
#include "OpenLCBHeader.h"        // System house-keeping.
#include <ESP32Servo.h>

#define OLCB_NO_BLUE_GOLD // Do not delete

// Define Frog Relay Output Pins
#define FROG_PIN_0  25  // Frog relay for Servo 0 (Servo Pin 32)
#define FROG_PIN_1  26  // Frog relay for Servo 1 (Servo Pin 33)

// Define Discrete Pull-up Input Pins (6 inputs)
#define NUM_INPUTS 6
const uint8_t inputPins[NUM_INPUTS] = { 16, 17, 5, 18, 19, 21 }; 

// Define Discrete Matrix Output Pins (6 outputs)
#define NUM_OUTPUT_PINS 6
const uint8_t outputPins[NUM_OUTPUT_PINS] = { 22, 23, 13, 12, 14, 27 };

// Define Matrix Action Limits
#define NUM_ACTION_GROUPS 3
#define NUM_ACTIONS_PER_GROUP 10
#define TOTAL_ACTIONS (NUM_ACTION_GROUPS * NUM_ACTIONS_PER_GROUP) // 30 Actions total

// Runtime trackers for structural lighting effects
uint8_t outputPinState[NUM_OUTPUT_PINS] = { LOW, LOW, LOW, LOW, LOW, LOW }; 
uint8_t activeActionType[NUM_OUTPUT_PINS] = { 0, 0, 0, 0, 0, 0 }; // Active effect on pin
uint32_t outputTimer[NUM_OUTPUT_PINS] = { 0, 0, 0, 0, 0, 0 };     // General execution timer
uint32_t outputParam1[NUM_OUTPUT_PINS] = { 0, 0, 0, 0, 0, 0 };    // Cached On-delay (100ms ticks)
uint32_t outputParam2[NUM_OUTPUT_PINS] = { 0, 0, 0, 0, 0, 0 };    // Cached Off-delay (100ms ticks)
uint8_t strobeState[NUM_OUTPUT_PINS] = { 0, 0, 0, 0, 0, 0 };

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
        <name>Inputs Using INPUT_PULLUP To Hold The Pin HIGH 3.3 Volts.</name>
        <repname>Pin D16 </repname>
        <repname>Pin D17 </repname>
        <repname>Pin D5 </repname>
        <repname>Pin D18 </repname>
        <repname>Pin D19 </repname>
        <repname>Pin D21 </repname>
        <string size='24'><name>Input Description / Location</name></string>
        
        <int size='1'>
            <name>Input Operating Mode</name>
            <description>Select how physical pin changes trigger the Event IDs.</description>
            <min>0</min><max>1</max>
            <map>
                <relation><property>0</property><value>Direct State Tracking (Sensor / Switch)</value></relation>
                <relation><property>1</property><value>Pushbutton Toggle State Change</value></relation>
            </map>
        </int>

        <int size='1'>
          <name>On-Delay / Transit LOW (0 to 25.5 seconds)</name>
          <description>Value multiplied by 100ms. Time signal must stay LOW before event transmits.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <int size='1'>
          <name>Off-Delay / Transit HIGH (0 to 25.5 seconds)</name>
          <description>Value multiplied by 100ms. Time signal must stay HIGH before event transmits.</description>
          <min>0</min><max>255</max>
          <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
        </int>

        <eventid><name>Input Transited HIGH Event </name></eventid>
        <eventid><name>Input Transited LOW Event </name></eventid>
    </group>
    
    <group replication=')" N(NUM_ACTION_GROUPS) R"('>
        <name>Output Matrix Routing Configuration</name>
        <description>Configure shared matrix execution parameters across the outputs.</description>
        <repname>Actions 1-10</repname>
        <repname>Actions 11-20</repname>
        <repname>Actions 21-30</repname>
        
        <group replication=')" N(NUM_ACTIONS_PER_GROUP) R"('>
            <name>Actions </name>
            <repname>Action </repname>
            <string size='24'><name>Description</name></string>
            <eventid><name>Consumed Activation Event ID</name></eventid>
            
            <int size='1'>
                <name>Target Hardware Logic Pin</name>
                <description>Choose the physical execution pin on the board.</description>
                <min>0</min><max>6</max>
                <map>
                    <relation><property>0</property><value>None / Inactive</value></relation>
                    <relation><property>1</property><value>Pin D22</value></relation>
                    <relation><property>2</property><value>Pin D23</value></relation>
                    <relation><property>3</property><value>Pin D13</value></relation>
                    <relation><property>4</property><value>Pin D12</value></relation>
                    <relation><property>5</property><value>Pin D14</value></relation>
                    <relation><property>6</property><value>Pin D27</value></relation>
                </map>
            </int>
            
            <int size='1'>
                <name>Output Lighting Effect Type</name>
                <min>0</min><max>5</max>
                <map>
                    <relation><property>0</property><value>None (Idle Output)</value></relation>
                    <relation><property>1</property><value>Drive LOW (Ground Pin)</value></relation>
                    <relation><property>2</property><value>Drive HIGH (Supply 3.3V)</value></relation>
                    <relation><property>3</property><value>Asynchronous Flashing</value></relation>
                    <relation><property>4</property><value>Double Strobe Flash Effect</value></relation>
                    <relation><property>5</property><value>Random Flicker Emulation</value></relation>
                </map>
            </int>
            
            <int size='1'>
                <name>Parameter 1: On-Duration Delay</name>
                <description>0 = Steady State. 1-255 Multiplied by 100ms baseline duration windows.</description>
                <min>0</min><max>255</max>
                <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
            </int>

            <int size='1'>
                <name>Parameter 2: Off-Duration / Interval Delay</name>
                <description>0 = Single Shot execution. 1-255 Multiplied by 100ms baseline duration windows.</description>
                <min>0</min><max>255</max>
                <hints><slider tickSpacing='65' immediate='yes' showValue='yes'> </slider></hints>
            </int>
        </group>
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

      struct {
        char desc[24]; 
        uint8_t mode;     
        uint8_t onDelay;  
        uint8_t offDelay; 
        EventID highStateEid;
        EventID lowStateEid;
      } inputs[NUM_INPUTS];

      // Matrix Routing Action Engine memory definitions (30 total)
      struct {
        char desc[24];
        EventID consumeEid;
        uint8_t targetPinIndex; // 0=None, 1=D22, 2=D23, 3=D13, 4=D12, 5=D14, 6=D27
        uint8_t effectType;     // 0=None, 1=Low, 2=High, 3=Flash, 4=Strobe, 5=Random
        uint8_t param1;         // On Delay duration scale
        uint8_t param2;         // Off Delay duration scale
      } actions[TOTAL_ACTIONS];

  uint8_t curpos[NUM_SERVOS]; 
} MemStruct;                

uint8_t curpos[NUM_SERVOS]; 

// Dynamic tracker configurations
bool servoMoving[NUM_SERVOS] = {false, false};
bool midCrossed[NUM_SERVOS] = {false, false}; 

bool lastInputState[NUM_INPUTS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
bool stableInputState[NUM_INPUTS] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; 
bool virtualToggleState[NUM_INPUTS] = {false, false, false, false, false, false}; 
uint32_t inputTimer[NUM_INPUTS] = {0, 0, 0, 0, 0, 0};

const uint8_t frogPins[NUM_SERVOS] = { FROG_PIN_0, FROG_PIN_1 };

extern "C" {
    // Total registered Node events: 14 (Servos) + 12 (Inputs) + 30 (Matrix Consumers) = 56
    const EIDTab eidtab[NUM_EVENT] PROGMEM = {
        // ================= SERVO 0 (7 Events) =================
        CEID(servos[0].pos[0].eid), CEID(servos[0].pos[1].eid), CEID(servos[0].pos[2].eid),          
        PEID(servos[0].reachedClosedEid), PEID(servos[0].reachedThrownEid),    
        PEID(servos[0].passedMidThrownEid), PEID(servos[0].passedMidClosedEid),  

        // ================= SERVO 1 (7 Events) =================
        CEID(servos[1].pos[0].eid), CEID(servos[1].pos[1].eid), CEID(servos[1].pos[2].eid),          
        PEID(servos[1].reachedClosedEid), PEID(servos[1].reachedThrownEid),    
        PEID(servos[1].passedMidThrownEid), PEID(servos[1].passedMidClosedEid),

        // ================= INPUTS 0-5 (12 Events) =================
        PEID(inputs[0].highStateEid), PEID(inputs[0].lowStateEid),
        PEID(inputs[1].highStateEid), PEID(inputs[1].lowStateEid),
        PEID(inputs[2].highStateEid), PEID(inputs[2].lowStateEid),
        PEID(inputs[3].highStateEid), PEID(inputs[3].lowStateEid),
        PEID(inputs[4].highStateEid), PEID(inputs[4].lowStateEid),
        PEID(inputs[5].highStateEid), PEID(inputs[5].lowStateEid),

        // ================= MATRIX ROUTING ACTIONS (30 Consumer Events) =================
        CEID(actions[0].consumeEid),  CEID(actions[1].consumeEid),  CEID(actions[2].consumeEid),  CEID(actions[3].consumeEid),  CEID(actions[4].consumeEid),
        CEID(actions[5].consumeEid),  CEID(actions[6].consumeEid),  CEID(actions[7].consumeEid),  CEID(actions[8].consumeEid),  CEID(actions[9].consumeEid),
        CEID(actions[10].consumeEid), CEID(actions[11].consumeEid), CEID(actions[12].consumeEid), CEID(actions[13].consumeEid), CEID(actions[14].consumeEid),
        CEID(actions[15].consumeEid), CEID(actions[16].consumeEid), CEID(actions[17].consumeEid), CEID(actions[18].consumeEid), CEID(actions[19].consumeEid),
        CEID(actions[20].consumeEid), CEID(actions[21].consumeEid), CEID(actions[22].consumeEid), CEID(actions[23].consumeEid), CEID(actions[24].consumeEid),
        CEID(actions[25].consumeEid), CEID(actions[26].consumeEid), CEID(actions[27].consumeEid), CEID(actions[28].consumeEid), CEID(actions[29].consumeEid)
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
  dP("\n 2Servos, 6 Inputs & 30 Routing Matrix Lighting Actions Loaded.");
  dP("\nNode ID="); dP(TOSTRING((NODE_ADDRESS)));
}

void userInitAll()
{ 
  NODECONFIG.put(EEADDR(nodeName), ESTRING("Esp32"));
  NODECONFIG.put(EEADDR(nodeDesc), ESTRING("ServosInputsOutputs"));
  NODECONFIG.update(SERVO_DELAY_OFFSET, 20);
  
  for(uint8_t i = 0; i < NUM_SERVOS; i++) {
    NODECONFIG.put(EEADDR(servos[i].desc), ESTRING(""));
    for(int p=0; p<NUM_POS; p++) {
      NODECONFIG.update(EEADDR(servos[i].pos[p].angle), 90);
    }
  }

  for(uint8_t i = 0; i < NUM_INPUTS; i++) {
    NODECONFIG.put(EEADDR(inputs[i].desc), ESTRING(""));
    NODECONFIG.update(EEADDR(inputs[i].mode), 0);     
    NODECONFIG.update(EEADDR(inputs[i].onDelay), 0);  
    NODECONFIG.update(EEADDR(inputs[i].offDelay), 0); 
  }

  // Clear Matrix Actions mapping structure configuration blocks
  for(uint8_t i = 0; i < TOTAL_ACTIONS; i++) {
    NODECONFIG.put(EEADDR(actions[i].desc), ESTRING(""));
    NODECONFIG.update(EEADDR(actions[i].targetPinIndex), 0); // None
    NODECONFIG.update(EEADDR(actions[i].effectType), 0);     // None
    NODECONFIG.update(EEADDR(actions[i].param1), 0);
    NODECONFIG.update(EEADDR(actions[i].param2), 0);
  }
  
  EEPROMcommit;
}

enum evStates { VALID=4, INVALID=5, UNKNOWN=7 };

uint8_t userState(uint16_t index) {
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
    else if (index < (NUM_SERVOS * 7) + (NUM_INPUTS * 2)) {
        int inputIdx = (index - (NUM_SERVOS * 7)) / 2;
        int stateType = (index - (NUM_SERVOS * 7)) % 2; 
        
        uint8_t operationalMode = NODECONFIG.read(EEADDR(inputs[inputIdx].mode));
        if (operationalMode == 0) {
            bool trackingState = stableInputState[inputIdx];
            if (stateType == 0 && trackingState == HIGH) return VALID;
            if (stateType == 1 && trackingState == LOW) return VALID;
        } else {
            bool trackingState = virtualToggleState[inputIdx];
            if (stateType == 0 && trackingState == false) return VALID; 
            if (stateType == 1 && trackingState == true) return VALID;  
        }
        return INVALID;
    }
    
    // Matrix routing actions are consumer events; their configuration status returns UNKNOWN
    return UNKNOWN;
}  

void pceCallback(uint16_t index) {
    dP("\npceCallback, index="); dP((uint16_t)index);
    
    // 1. Check if Event is a Turnout Command
    if (index < (NUM_SERVOS * 7)) {
        int ch = index / 7;
        int localIndex = index % 7;
        if (ch < NUM_SERVOS && localIndex < 3) {
            curpos[ch] = localIndex;
            servoTarget[ch] = NODECONFIG.read( EEADDR(servos[ch].pos[localIndex].angle) );
            servoMoving[ch] = true; 
            midCrossed[ch] = false; 
        }
        return;
    }
    
    // 2. Evaluate Matrix Routing Consumers
    uint16_t baseMatrixIndex = (NUM_SERVOS * 7) + (NUM_INPUTS * 2);
    if (index >= baseMatrixIndex && index < NUM_EVENT) {
        uint16_t actionIdx = index - baseMatrixIndex;
        
        uint8_t chosenPinMap = NODECONFIG.read(EEADDR(actions[actionIdx].targetPinIndex));
        if (chosenPinMap >= 1 && chosenPinMap <= NUM_OUTPUT_PINS) {
            uint8_t targetPinArrayOffset = chosenPinMap - 1; // Align to 0-5 tracking indices
            
            // Map configuration rules directly down onto hardware cache parameters
            activeActionType[targetPinArrayOffset] = NODECONFIG.read(EEADDR(actions[actionIdx].effectType));
            outputParam1[targetPinArrayOffset] = NODECONFIG.read(EEADDR(actions[actionIdx].param1));
            outputParam2[targetPinArrayOffset] = NODECONFIG.read(EEADDR(actions[actionIdx].param2));
            outputTimer[targetPinArrayOffset] = 0; // Force immediate calculation execution pass
            
            dP("\n [Matrix Active] Action #"); dP(actionIdx); dP(" triggered Array Offset Pin Index: "); dP(targetPinArrayOffset);
        }
    }
}

void userSoftReset() {}
void userHardReset() {}

NodeID nodeid(NODE_ADDRESS);  
#include "OpenLCBMid.h"    

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
          if (curpos[i] == 0) OpenLcb.produce(servoBaseIndex + 3); 
          else if (curpos[i] == 2) OpenLcb.produce(servoBaseIndex + 4); 
        }
        continue;
      }
      
      if(servoTarget[i] > servoActual[i]) {
        if ((servoTarget[i] - servoActual[i]) > stepSize) servoActual[i] += stepSize;
        else servoActual[i] = servoTarget[i]; 
      }
      else if(servoTarget[i] < servoActual[i]) {
        if ((servoActual[i] - servoTarget[i]) > stepSize) servoActual[i] -= stepSize;
        else servoActual[i] = servoTarget[i]; 
      }

      if (servoMoving[i] && !midCrossed[i]) {
        if (curpos[i] == 2) { 
          if ((oldActual < midAngle && servoActual[i] >= midAngle) || (oldActual > midAngle && servoActual[i] <= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], HIGH); 
             OpenLcb.produce((i * 7) + 5);
          }
        }
        else if (curpos[i] == 0) {
          if ((oldActual > midAngle && servoActual[i] <= midAngle) || (oldActual < midAngle && servoActual[i] >= midAngle)) {
             midCrossed[i] = true;
             digitalWrite(frogPins[i], LOW);  
             OpenLcb.produce((i * 7) + 6);
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

void inputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(100)); 
    
    for(int i = 0; i < NUM_INPUTS; i++) {
      bool currentReading = digitalRead(inputPins[i]);
      uint8_t operationalMode = NODECONFIG.read(EEADDR(inputs[i].mode));
      uint16_t inputBaseIndex = (NUM_SERVOS * 7) + (i * 2);

      if (operationalMode == 0) {
        if (currentReading != lastInputState[i]) {
          lastInputState[i] = currentReading;
          uint8_t configurationDelay = (currentReading == LOW) ? NODECONFIG.read(EEADDR(inputs[i].onDelay)) : NODECONFIG.read(EEADDR(inputs[i].offDelay));
          inputTimer[i] = configurationDelay; 
        } 
        else if (currentReading != stableInputState[i]) {
          if (inputTimer[i] > 0) inputTimer[i]--;
          if (inputTimer[i] == 0) {
            stableInputState[i] = currentReading; 
            if (stableInputState[i] == HIGH) OpenLcb.produce(inputBaseIndex); 
            else OpenLcb.produce(inputBaseIndex + 1); 
          }
        }
      } 
      else {
        if (currentReading != lastInputState[i]) {
          lastInputState[i] = currentReading;
          if (currentReading == LOW) {
            uint8_t valDelay = NODECONFIG.read(EEADDR(inputs[i].onDelay));
            inputTimer[i] = valDelay;
            
            if (valDelay == 0 && stableInputState[i] == HIGH) {
              stableInputState[i] = LOW; 
              virtualToggleState[i] = !virtualToggleState[i];
              if (virtualToggleState[i] == true) OpenLcb.produce(inputBaseIndex + 1); 
              else OpenLcb.produce(inputBaseIndex); 
              inputTimer[i] = NODECONFIG.read(EEADDR(inputs[i].offDelay));
            }
          } 
          else {
            if (stableInputState[i] == LOW) stableInputState[i] = HIGH;
            else inputTimer[i] = 0;
          }
        } 
        else if (currentReading == LOW && stableInputState[i] == HIGH) {
          if (inputTimer[i] > 0) inputTimer[i]--;
          if (inputTimer[i] == 0) {
            stableInputState[i] = LOW; 
            virtualToggleState[i] = !virtualToggleState[i];
            if (virtualToggleState[i] == true) OpenLcb.produce(inputBaseIndex + 1); 
            else OpenLcb.produce(inputBaseIndex); 
            inputTimer[i] = NODECONFIG.read(EEADDR(inputs[i].offDelay));
          }
        }
        else {
          if (inputTimer[i] > 0) inputTimer[i]--;
        }
      }
    }
  }
}

// Background core execution engine manipulating matrix layout lighting effects
void outputBackgroundTask(void * parameter) {
  for(;;) {
    vTaskDelay(pdMS_TO_TICKS(100)); // 100ms baseline timing resolution
    
    for(int i = 0; i < NUM_OUTPUT_PINS; i++) {
      uint8_t pinHex = outputPins[i];
      uint8_t effect = activeActionType[i];
      
      switch(effect) {
        case 0: // Mode: None (Force Idle Low)
          outputPinState[i] = LOW;
          break;
          
        case 1: // Mode: Drive Pin LOW
          outputPinState[i] = LOW;
          break;
          
        case 2: // Mode: Drive Pin HIGH
          outputPinState[i] = HIGH;
          break;
          
        case 3: // Mode: Flashing Engine
          if (outputTimer[i] > 0) {
            outputTimer[i]--;
          } else {
            outputPinState[i] = !outputPinState[i]; // Flip lighting channel state
            outputTimer[i] = (outputPinState[i] == HIGH) ? outputParam1[i] : outputParam2[i];
            if (outputTimer[i] == 0) outputTimer[i] = 5; // Enforce fallback safety window (500ms)
          }
          break;
          
case 4: // Mode: True Double Strobe Flash Effect
          if (outputTimer[i] > 0) {
            outputTimer[i]--;
          } else {
            // strobeState array is reused here as a step sequence counter (0 to 3)
            // We use outputParam1 for flash duration (default 1 tick / 100ms if 0)
            uint32_t flashDuration = (outputParam1[i] == 0) ? 1 : outputParam1[i];
            uint32_t longRestDuration = (outputParam2[i] == 0) ? 10 : outputParam2[i]; // Default 1 sec rest

            switch(strobeState[i]) {
              case 0: // 1st Flash ON
                outputPinState[i] = HIGH;
                outputTimer[i] = flashDuration;
                strobeState[i] = 1; // Advance to short gap
                break;

              case 1: // Short Gap OFF
                outputPinState[i] = LOW;
                outputTimer[i] = 1; // Hardcoded 100ms crisp separation gap
                strobeState[i] = 2; // Advance to 2nd flash
                break;

              case 2: // 2nd Flash ON
                outputPinState[i] = HIGH;
                outputTimer[i] = flashDuration;
                strobeState[i] = 3; // Advance to long rest period
                break;

              case 3: // Long Rest Period OFF
                outputPinState[i] = LOW;
                outputTimer[i] = longRestDuration;
                strobeState[i] = 0; // Reset back to 1st flash for next cycle
                break;
                
              default:
                strobeState[i] = 0;
                break;
            }
          }
          break;
          
case 5: // Mode: Organic Random Flicker (Welding / Fire Simulator)
          if (outputTimer[i] > 0) {
            outputTimer[i]--;
          } else {
            // Read parameters and ensure a minimum scaling factor of 1 to prevent division/zero issues
            uint32_t durn = (outputParam1[i] == 0) ? 1 : outputParam1[i]; // Controls ON duration
            uint32_t rate = (outputParam2[i] == 0) ? 1 : outputParam2[i]; // Controls OFF duration

            if (strobeState[i] == 0) {
              // --- Turn Pin HIGH ---
              outputPinState[i] = HIGH;
              
              // Map the non-linear random duration formula into loop ticks
              // Min ticks: (durn * durn * 2) / 10, Max ticks: (durn * durn * 8) / 10
              uint32_t minTicks = (durn * durn * 2) / 10;
              uint32_t maxTicks = (durn * durn * 8) / 10;
              if (minTicks < 1) minTicks = 1;
              if (maxTicks <= minTicks) maxTicks = minTicks + 2;

              outputTimer[i] = minTicks + (esp_random() % (maxTicks - minTicks));
              strobeState[i] = 1; // Next step is to turn it LOW
              
              dP("\n Flicker ON ticks: "); dP(outputTimer[i]);
            } 
            else {
              // --- Turn Pin LOW ---
              outputPinState[i] = LOW;
              
              // Map the non-linear random interval rate formula into loop ticks
              uint32_t minTicks = (rate * rate * 2) / 10;
              uint32_t maxTicks = (rate * rate * 8) / 10;
              if (minTicks < 1) minTicks = 1;
              if (maxTicks <= minTicks) maxTicks = minTicks + 2;

              outputTimer[i] = minTicks + (esp_random() % (maxTicks - minTicks));
              strobeState[i] = 0; // Next step is to turn it HIGH
              
              dP("\n Flicker OFF ticks: "); dP(outputTimer[i]);
            }
          }
          break;
      }
      
      digitalWrite(pinHex, outputPinState[i]);
    }
  }
}

void servoStartUp() {
  for(int i=0; i<NUM_SERVOS; i++) {
    curpos[i] = 0; 
    digitalWrite(frogPins[i], LOW);
    servoActual[i] = NODECONFIG.read( EEADDR( servos[i].pos[curpos[i]].angle ) );
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

  pinMode(FROG_PIN_0, OUTPUT);
  pinMode(FROG_PIN_1, OUTPUT);

  for(int i = 0; i < NUM_INPUTS; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
    lastInputState[i] = digitalRead(inputPins[i]); 
    stableInputState[i] = lastInputState[i];
  }

  // Initialize Matrix Output Pins
  for(int i = 0; i < NUM_OUTPUT_PINS; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], LOW);
  }

  EEPROMbegin;
  NodeID nodeid(NODE_ADDRESS);      
  Olcb_init(nodeid, RESET_TO_FACTORY_DEFAULTS);
  reportConfig();

  servoStartUp();

  xTaskCreatePinnedToCore(servoBackgroundTask, "ServoTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(inputBackgroundTask, "InputTask", 3072, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(outputBackgroundTask, "OutputTask", 3072, NULL, 1, NULL, 0);
}

void loop() {
  Olcb_process();        
}
