//The Goal of this code is to Serialize a CAN Frame into an array of 16 bytes. Wish me luck!

#include <SPI.h>
#include <FlexCAN.h>
#include <CryptoAccel.h>

//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define message structure from FlexCAN library
static CAN_message_t rxmsg;
static CAN_message_t rxmsg1;
static CAN_message_t txmsg;

//Utility Variables
unsigned int i;

//CAN Frame variables
uint16_t fTime;
uint32_t fullID;
uint8_t dlc;

//LED indicator variables
const int wLED = 16;
const int rLED = 5;
boolean wLED_state;
boolean rLED_state;


//A generic CAN Frame print function for the Serial terminal
void printFrame(CAN_message_t rxmsg, uint8_t channel, uint32_t RXCount)
{
  char CANdataDisplay[50];
  sprintf(CANdataDisplay, "%lu %12l %12l %08l %l %l", channel, RXCount, micros(), rxmsg.id, rxmsg.ext, rxmsg.len);
  Serial.print(CANdataDisplay);
  for (uint8_t i = 0; i < rxmsg.len; i++) {
    char CANBytes[4];
    sprintf(CANBytes, " %02X", rxmsg.buf[i]);
    Serial.print(CANBytes);
  }
  Serial.println();
}

void setup() {
  //LED Code for visualization
  pinMode(wLED, OUTPUT);
  pinMode(rLED, OUTPUT);
  wLED_state = true;
  rLED_state = true;
  digitalWrite(wLED, wLED_state);
  digitalWrite(rLED, rLED_state);
  while(!Serial);
  Serial.println("Starting CAN test.");
  
  //Initialize the CAN channels with optional second channel if K66 processor (Teensy 3.6) Legacy feature from Teensy 3.2
  Can0.begin(0);
  Can1.begin(0);
  
  static CAN_message_t rxmsg0;
  
  //Turn on MCP 2562 Chips
  pinMode(14, OUTPUT); //CAN0
  pinMode(35, OUTPUT); //CAN1
  digitalWrite(14, LOW);
  digitalWrite(35, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(Can0.read(rxmsg)){
    uint8_t channel = 0; //1 byte
    uint16_t fTime = rxmsg.timestamp; //2 bytes
    byte fTimebyte1 =
    byte fTimebyte2 = 
    uint32_t fullID = rxmsg << 3; //4 bytes
    byte fullIDbyte1 =
    byte fullIDbyte2 =
    byte fullIDbyte3 =
    byte fullIDbyte4 =
    uint8_t dlc = rxmsg.len; //1 byte   
    byte sFrame[16] = 
    { 
      channel, 
      fTimebyte1, fTimebyte2, 
      fullIDbyte1, fullIDbyte2, fullIDbyte3, fullIDbyte4, 
      dlc, 
      rxmsg.buf[0], rxmsg.buf[1], rxmsg.buf[2], rxmsg.buf[3], rxmsg.buf[4], rxmsg.buf[5], rxmsg.buf[6], rxmsg.buf[7]
    };
    
  }

}
