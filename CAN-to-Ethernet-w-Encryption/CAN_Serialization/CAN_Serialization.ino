#include <SPI.h>
#include <FlexCAN.h>
#include <CryptoAccel.h>

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

}
