//Libraries
#include <SPI.h>
#include <FlexCAN.h>
//Create a counter to keep track of message traffic
uint32_t TXCount = 0;

//Define CAN message data type
static CAN_message_t txmsg;
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

void setup()
{
  pinMode(14, OUTPUT);
  pinMode(35, OUTPUT);
  digitalWrite(35, LOW);
  digitalWrite(14, LOW);
  while(!Serial);
  Serial.println("Setting up CAN channels:");
  //Set up CAN 
  Can0.begin(0);
  Serial.println("CAN 0 Begin");
  Can1.begin(0);
  Serial.println("CAN 1 Begin");
  
  //Turn on LED
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);
  Serial.println("Starting CAN test.");
  txmsg.id = 0xABCDEF00;
  txmsg.ext = 1;
  txmsg.len = 8;
  txmsg.buf[0] = 0x11;
  txmsg.buf[1] = 0x22;
  txmsg.buf[2] = 0x33;
  txmsg.buf[3] = 0x44;
  txmsg.buf[4] = 0x55;
  txmsg.buf[5] = 0x66;
  txmsg.buf[6] = 0x77;
  txmsg.buf[7] = 0x88;
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(500);
  while (true){
    if (TXCount % 2 == 0){
      Can0.write(txmsg);
    }
    else{
      Can1.write(txmsg);
    }
    if(Can0.read(rxmsg0)){
      printFrame(rxmsg0, 0, RXCount0++);
    }
    if(Can1.read(rxmsg1)){
      printFrame(rxmsg1, 0, RXCount0++);
    }
    TXCount++;
  }
}
