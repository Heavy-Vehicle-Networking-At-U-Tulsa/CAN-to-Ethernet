/*
 * Vehicle Communcations CAN and PWM to UDP 
 *  
 * Aruino Sketch for the CAN to Ethernet board to transmit CAN frames within Ethernet packets
 * 
 * Written By Jeremy Daily
 * The University of Tulsa
 * 
 * This program records all received CAN Frames to a buffer, and then transmits the data over ethernet.  
 * 
 *
 * 
 */


#include <Ethernet.h>
#include <EthernetUdp.h>
#include <FlexCAN.h>


elapsedMillis heartbeat_timer;

const int pwm1Pin = 20;
const int pwm2Pin = 21;
const int pwm3Pin = 22;

#define UDP_TX_PACKET_MAX_SIZE 1450

byte EthBuffer[1450];

//Initialize the parameters for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02 };
byte ip[] = { 192, 168, 1, 2 }; // Assigned by DHCP
byte server[] = { 192, 168, 1, 3 }; // Python Server IP
uint16_t port = 59581;

EthernetUDP Udp;


// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";        // a string to send back


//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define message structure from FlexCAN library
static CAN_message_t rxmsg;
static CAN_message_t txmsg;

boolean LED_state;
////A generic CAN Frame print function for the Serial terminal
//void printFrame(CAN_message_t rxmsg, uint8_t channel, uint32_t RXCount)
//{
//  char CANdataDisplay[50];
//  sprintf(CANdataDisplay, "%d %12lu %12lu %08X %d %d", channel, RXCount, micros(), rxmsg.id, rxmsg.ext, rxmsg.len);
//  Serial.print(CANdataDisplay);
//  for (uint8_t i = 0; i < rxmsg.len; i++) {
//    char CANBytes[4];
//    sprintf(CANBytes, " %02X", rxmsg.buf[i]);
//    Serial.print(CANBytes);
//  }
//  Serial.println();
//}



//Arduino Setup
void setup()
{
  // Start of setup. Code that only runs once
  Serial.println("Made it to setup!");
  
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
  pinMode(pwm1Pin, OUTPUT);
  pinMode(pwm2Pin, OUTPUT);
  pinMode(pwm3Pin, OUTPUT);

  
  //LED Code for visualization
  pinMode(LED_BUILTIN, OUTPUT);
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);

  Serial.println("Starting Ethernet.");
  
  //Ethernet Code: This is sitting in setup, but I'm thinking about making it a function and including it in the loop so that reconnection is possible. Makes the code more robust
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(localPort);
   
  
  
  Serial.println("Connecting to Ethernet Server");

//  if (client.connect(server, )) {
//    Serial.println("connected");
//  } else {
//    Serial.println("connection failed");
//  }
  //Initialize the CAN channels with autobaud setting
  Can0.begin(0);
  #if defined(__MK66FX1M0__)
  Can1.begin(0);
  #endif
  
  
}
int location = 0;
uint32_t heartbeat_counter;

uint32_t heartbeat_millis;

char message[] = "I'm not Dead ";

void send_heartbeat(){
  heartbeat_timer = 0;
  byte n = sizeof(message);
  EthBuffer[0] = n;
  memcpy(&EthBuffer[1],&message[0],n);
  heartbeat_counter++;
  memcpy(&EthBuffer[n+1],&heartbeat_counter,4);
  heartbeat_millis = millis();
  memcpy(&EthBuffer[n+5],&heartbeat_millis,4);
  Udp.beginPacket(server, port);
  Udp.write(EthBuffer,n+9);
  Udp.endPacket();
    
  for (int i=0;i<n+9;i++){
    Serial.print(EthBuffer[i],HEX);
    Serial.print(" ");  
  }
  Serial.println();
  
}


void loop()
{
  if (heartbeat_timer > 2000) send_heartbeat();
  
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i=0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, packetSize);
    Serial.println("Contents:");
    Serial.println(packetBuffer);
    memset(packetBuffer,0x00,packetSize);
    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
  
  while (Can0.read(rxmsg)) { // Can 0 Teensy is Can1 Beagle
//    if (pOut <= 255){
//      analogWrite(pwmPin, pOut);
//      pOut++;
//    }
//    else{
//      pOut = 245;
//      analogWrite(pwmPin, pOut);
//      pOut++;
//    }
//    if (rxmsg.id == 0x08FE6E0B){
//      printFrame(rxmsg,0,RXCount0++);
//    }
    
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
      Udp.write(EthBuffer,1450);
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
//    printFrame(rxmsg,1,RXCount1++);
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
   }
  #endif
}
