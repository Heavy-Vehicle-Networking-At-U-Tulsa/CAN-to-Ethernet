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


#define HEARTBEAT_MESSAGE_PERIOD_MS 5000
#define CAN_SEND_TIMEOUT_MS 100
#define MAX_MESSAGE_COUNT 25
#define CAN_MESSAGE_SIZE 25

uint8_t message_count = 0;

elapsedMillis heartbeat_timer;
elapsedMillis CAN_send_timer;
uint32_t heartbeat_counter;


const int pwm1Pin = 20;
const int pwm2Pin = 21;
const int pwm3Pin = 22;
const int DAC0Pin = A21;
const int DAC1Pin = A22;

#define PACKET_MAX_SIZE 1450
byte EthBuffer[PACKET_MAX_SIZE];

//Initialize the parameters for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02 };
byte ip[] = { 192, 168, 1, 2 }; // Assigned by DHCP
byte server[] = { 192, 168, 1, 3 }; // Python Server IP
uint16_t port = 59581;

EthernetUDP Udp;

int location = 0;

// buffers for receiving and sending data
char packetBuffer[PACKET_MAX_SIZE];  // buffer to hold incoming packet,


//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define message structure from FlexCAN library
static CAN_message_t rxmsg;
static CAN_message_t txmsg;

boolean LED_state;

//Arduino Setup
void setup()
{
  // Start of setup. Code that only runs once
  Serial.print("Max UDP Frame Size: ");
  Serial.println(PACKET_MAX_SIZE);
  
  //PWM Setup Code:
  pinMode(pwm1Pin, OUTPUT);
  pinMode(pwm2Pin, OUTPUT);
  pinMode(pwm3Pin, OUTPUT);
  pinMode(DAC0Pin, OUTPUT);
  analogWrite(DAC0Pin,2047); // Mid range of 12-bit resolution
  pinMode(DAC1Pin, OUTPUT);
  analogWrite(DAC1Pin,2047); // Mid range of 12-bit resolution
  
  //LED Code for visualization
  pinMode(LED_BUILTIN, OUTPUT);
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);

  Serial.println("Starting Ethernet.");
  
  //Ethernet Code: This is sitting in setup, but I'm thinking about making it a function and including it in the loop so that reconnection is possible. Makes the code more robust
  Ethernet.begin(mac, ip);
  
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware.");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start UDP
  Udp.begin(port);
  
  //Initialize the CAN channels with autobaud setting
  Can0.begin(0);
  Serial.println("CAN0 Started");
  #if defined(__MK66FX1M0__)
  Can1.begin(0);
  Serial.println("CAN1 Started");
  #endif
}
/*
 * The heartbeat signal is a simple test to show there is basic connectivity. 
 * This should be transmitted infreqently
 */


void send_heartbeat(){
  char message[] = "I'm not Dead ";
  heartbeat_timer = 0;
  byte n = sizeof(message);
  EthBuffer[0] = n;
  memcpy(&EthBuffer[1],&message[0],n);
  heartbeat_counter++;
  memcpy(&EthBuffer[n+1],&heartbeat_counter,4);
  uint32_t heartbeat_millis = millis();
  memcpy(&EthBuffer[n+5],&heartbeat_millis,4);
  Udp.beginPacket(server, port);
  Udp.write(EthBuffer,n+9);
  Udp.endPacket();
}

void send_CAN_buffer(){
  CAN_send_timer = 0;
  location = 1;
  EthBuffer[0] = message_count;
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  byte sent_bytes = Udp.write(EthBuffer,message_count*CAN_MESSAGE_SIZE+1);
  Udp.endPacket();
  Serial.print("Num of bytes for CAN data: ");
  Serial.println(sent_bytes);
}

void load_ethernet_buffer(uint8_t can_channel){
   //Start of buffer loading
    // ID 32
    message_count++; // increment the count by 1
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
    // CAN Channel
    EthBuffer[location]=uint8_t(can_channel);
    location++;
    // len 8
    EthBuffer[location] = uint8_t(rxmsg.len);
    location++;
    // buf 8 byte array
    for (int i=0;i<=7; i++){
      EthBuffer[location] = rxmsg.buf[i];
      location++;
    }
    if (location >= MAX_MESSAGE_COUNT * CAN_MESSAGE_SIZE){
      send_CAN_buffer()
    }
}

void loop()
{
  if (heartbeat_timer > HEARTBEAT_MESSAGE_PERIOD_MS) send_heartbeat();

  if (CAN_send_timer > CAN_SEND_TIMEOUT_MS) send_CAN_buffer();
  
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
    // Reset the buffer
    memset(packetBuffer,0x00,packetSize);
    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
  
  while (Can0.read(rxmsg)) {
    //Blink the LED to show CAN traffic
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
    CAN_channel = 0;
    load_ethernet_buffer(CAN_channel);
    
   
  }

  #if defined(__MK66FX1M0__)
  while (Can1.read(rxmsg1)) {
//    printFrame(rxmsg1,1,RXCount1++);
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
   }
  #endif
}
