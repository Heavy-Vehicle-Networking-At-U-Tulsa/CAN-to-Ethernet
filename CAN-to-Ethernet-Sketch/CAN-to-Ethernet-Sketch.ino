
/*
 * Vehicle Communcations CAN to Ethernet Project
 * 
 * Aruino Sketch for the CAN to Ethernet board to transmit CAN frames within Ethernet packets
 * 
 * Written By Ben Ettlinger
 * The University of Tulsa
 * 
 * This program records all received CAN Frames to a buffer, and then transmits the data over ethernet.  
 * 
 * AutobaudForFlexCAN used to display CAN
 * 
 */
//My Idea this time is to build the CAN frame around ethernet, not the other way around
#include <Ethernet.h>
#include <SPI.h>
#include <FlexCAN.h>
#include <CryptoAccel.h>

const int pwmPin = 30;

//Initialize the parameters for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 1, 220 };
byte server[] = { 192, 168, 1, 184 }; // Python Server IP
uint8_t test[] = {44,44,44,44};
EthernetClient client;

//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define message structure from FlexCAN library
static CAN_message_t rxmsg;
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



//Arduino Setup
void setup()
{
  // Start of setup. Code that only runs once
  Serial.begin(9600);
  Serial.println("Made it to setup!");
  
  //Cryptography Code: We are going to use AES 256 bit encryption
  unsigned int i; // We will use this to parse later
  
  unsigned char aeskey[32] = 
  {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08
  }; 
  
  // Okay, I'm cheating here by using a preset key. Shoot me. 
  // Ideally, this key will be generated through a Diffie-Helmann Key Exchange within the connection phase of the server-client connection
  // For now, this works. The important thing is that the key is 256 bits, or 32 bytes. more specifically, an array of 32 bytes. This will be important later
  
  unsigned char keysched[4*60]; // Key schedule output. I don't really know how this works to be honest. I just know that for this data type
  mmcau_aes_set_key(aeskey, 256, keysched); //This is an AES key expansion. It accepts the variables that we just declared and it's output points to the keysched var

  //At this point we are prepared to do encryption of data. The data needs to be placed into 16 byte chunks. Current plan is to build the payload, encrypt it, then send it.
  
  //PWM Setup Code: Future project
  pinMode(pwmPin, OUTPUT);
  
  //LED Code for visualization
  pinMode(LED_BUILTIN, OUTPUT);
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);

  while(!Serial);
  Serial.println("Starting CAN test.");
  
  //Ethernet Code: This is sitting in setup, but I'm thinking about making it a function and including it in the loop so that reconnection is possible. Makes the code more robust
  Ethernet.begin(mac, ip);
  Serial.begin(9600);
  delay(1000);

  Serial.println("connecting...");

  if (client.connect(server, 59581)) {
    Serial.println("connected");
  } else {
    Serial.println("connection failed");
  }
  //Initialize the CAN channels with autobaud setting
  Can0.begin(0);
  #if defined(__MK66FX1M0__)
  Can1.begin(0);
  #endif
  
  
}
int location = 0;

byte EthBuffer[1450];
void loop()
{
  //while (true){
  //  client.write(42);
  //}
  long lastSend;
  int pOut = 245;
  while (Can0.read(rxmsg)) { // Can 0 Teensy is Can1 Beagle
    if (pOut <= 255){
      analogWrite(pwmPin, pOut);
      pOut++;
    }
    else{
      pOut = 245;
      analogWrite(pwmPin, pOut);
      pOut++;
    }
    if (rxmsg.id == 0x08FE6E0B){
      printFrame(rxmsg,0,RXCount0++);
    }
    
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
    //Start of buffer loading
    // ID 32
    EthBuffer[location] = uint8_t (rxmsg.id>>24); // Grabs the first 8 bits
    location++;
    EthBuffer[location] = uint8_t (rxmsg.id>>16); // grabs the last 8 bits of the first 16 bits
    location++;
    EthBuffer[location] = uint8_t (rxmsg.id>>8);  // Grabs the last 8 bits of the first 24 bits
    location++;
    EthBuffer[location] = uint8_t (rxmsg.id>>0);     // Grabs the last 8 bits of the ID
    location++;
    // Micros 32
    EthBuffer[location] = uint8_t (rxmsg.micros>>24);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.micros>>16);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.micros>>8);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.micros>>0);
    location++;
    // rxcount 32
    EthBuffer[location] = uint8_t (rxmsg.rxcount>>24);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.rxcount>>16);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.rxcount>>8);
    location++;
    EthBuffer[location] = uint8_t (rxmsg.rxcount>>0);
    location++;
    // timestamp 16
    EthBuffer[location] = uint8_t(rxmsg.timestamp>>8);
    location++;
    EthBuffer[location] = uint8_t(rxmsg.timestamp>>0);
    location++;
    //// flags
    //byte flag;
    //bitWrite(flag, 7, 
    //EthBuffer[location] = uint8_t(flag);
    // Instead of a flag, I'm going to indicate can Channel
    byte canChannel=0;
    EthBuffer[location]=uint8_t(canChannel);
    location++;
    // len 8
    EthBuffer[location] = uint8_t(rxmsg.len);
    location++;
    // buf 8 byte array
    for (int i=0;i<=7; i++){
      EthBuffer[location] = rxmsg.buf[i];
      location++;
    }
    EthBuffer[location]=0;
    location++;
    if (location >=1450){
      location = 0;
      client.write(EthBuffer,1450);
      long lastSend = millis();
    }
  }
  //if (!Can0.read(rxmsg)){
    //long currentTime = millis();
    //if (currentTime - lastSend >=5000){
    //  client.write(EthBuffer, location);
    //  client.stop();
    //}
  //}
  #if defined(__MK66FX1M0__)
  while (Can1.read(rxmsg)) {
    printFrame(rxmsg,1,RXCount1++);
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
   }
  #endif
}
