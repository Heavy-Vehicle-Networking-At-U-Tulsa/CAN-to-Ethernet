
//Libraries

#include <SPI.h>
#include <FlexCAN.h>

//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define CAN Message Type
static CAN_message_t rxmsg0;
static CAN_message_t rxmsg1;

//Define LED boolean
boolean LED_state;

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

//Setup block
//Code that only runs once
void setup()
{
  Serial.println("Setting up CAN channels");
  //Set up CAN 
  //Can0.begin(0);
  
  //Turn on LED
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);
  Serial.println("Starting CAN test.");
}

//Loop Block
void loop()
{
  while (Can0.read(rxmsg0)||Can1.read(rxmsg1)) {
    if(Can0.read(rxmsg0)){
      printFrame(rxmsg0, 0, RXCount0++);
    }
    if(Can1.read(rxmsg0)){
      printFrame(rxmsg1, 0, RXCount0++);
    }
  }
  
}
