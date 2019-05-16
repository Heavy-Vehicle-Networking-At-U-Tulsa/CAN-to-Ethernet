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
static CAN_message_t rxmsg0;
static CAN_message_t rxmsg1;
static CAN_message_t txmsg;

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

//CAN Frame Serializer that turns the CAN_message_t type into a 16 byte array of all important items
void serializeFrame(CAN_message_t rxmsg, uint8_t channel, uint8_t sFrame[16])
{
  //FlexCAN timestamp
  uint16_t fTime = rxmsg.timestamp; //2 bytes
  ////Serializing the timestamp
  uint8_t fTimebyte1 = fTime>>8;
  uint8_t fTimebyte2 = fTime;
  //Message ID
  ////We are going to shove the flag bits into the final 3 bits of the ID
  uint32_t fullID = rxmsg.id << 3; //4 bytes
  ////Here we create an 8 bit number to represent the flags (only really using final 3 bits)
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
  uint8_t fullIDbyte4 = fullID>>0 | flags; //Combining the final id byte with the flag byte
  uint8_t dlc = rxmsg.len; //1 byte
  sFrame[0] = channel;
  sFrame[1] = fTimebyte1;   sFrame[2] = fTimebyte2;
  sFrame[3] = fullIDbyte1;  sFrame[4] = fullIDbyte2;  sFrame[5] = fullIDbyte3;  sFrame[6] = fullIDbyte4;
  sFrame[7] = dlc;
  for (int j = 8; j<16; j++) sFrame[j] = rxmsg.buf[j-8];
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
     
  //Ethernet Code: 
  Serial.println("Starting Ethernet Connection");
  Ethernet.begin(mac, ip);
  delay(1000);
}

void loop() {
  //Ethernet Connection Code
  if (false){
  //if (!client.connected()){
    Serial.println("Attempting to connect to server");
    client.connect(server, 59581);
    if (client.connected()){
      rLED_state = true;
      digitalWrite(rLED, rLED_state);
      Serial.println("Connected!");
    }
    else{
      rLED_state = !rLED_state;
      digitalWrite(rLED, rLED_state);
      Serial.println("Connection Failed...");
    }
  } 
  else{
    uint8_t smsg[16];
    //Serial.println("Connected to Server. Printing CAN Messages");
    while(Can0.read(rxmsg0)){
      serializeFrame(rxmsg0, 0, smsg);
      for (int i = 0; i < sizeof(smsg); i++) Serial.print(smsg[i], HEX);
      Serial.println();
    }
  } 
}
