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
  sprintf(CANdataDisplay, "%d %12lu %12lu %08X %d %d", channel, RXCount, micros(), rxmsg.id, rxmsg.ext, rxmsg.len);
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
    printFrame(rxmsg, 0, RXCount0);
    //Channel Indicator
    uint8_t channel = 0; //1 byte
    //FlexCAN timestamp
    uint16_t fTime = rxmsg.timestamp; //2 bytes
    ////Serializing the timestamp
    uint8_t fTimebyte1 = fTime>>8;
    uint8_t fTimebyte2 = fTime;
    //Message ID
    ////We are going to shove the flag bits into the final 3 bits of the ID
    uint32_t fullID = rxmsg.id << 3; //4 bytes
    ////Here we create an 8 bit number to represent the 
    uint8_t flags;
    flags = flags | rxmsg.flags.extended;
    flags = flags<<1;
    flags = flags | rxmsg.flags.remote;
    flags = flags<<1;
    flags = flags | rxmsg.flags.overrun;
    ////Serializing the ID byte
    uint8_t fullIDbyte1 = fullID>>24;
    uint8_t fullIDbyte2 = fullID>>16;
    uint8_t fullIDbyte3 = fullID>>8;
    uint8_t fullIDbyte4 = fullID>>0 | flags;
    uint8_t dlc = rxmsg.len; //1 byte   
    uint8_t sFrame[16] = 
    { 
      channel, 
      fTimebyte1, fTimebyte2, 
      fullIDbyte1, fullIDbyte2, fullIDbyte3, fullIDbyte4, 
      dlc, 
      rxmsg.buf[0], rxmsg.buf[1], rxmsg.buf[2], rxmsg.buf[3], rxmsg.buf[4], rxmsg.buf[5], rxmsg.buf[6], rxmsg.buf[7]
    };
    for (int i = 0; i < sizeof(sFrame); i++) Serial.println((sFrame[i]));
    Serial.println();
    delay(3000);
  }

}
