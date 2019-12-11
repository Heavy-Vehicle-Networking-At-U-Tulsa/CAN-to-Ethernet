/*
   Vehicle Communcations CAN and PWM to UDP

   Aruino Sketch for the CAN to Ethernet board to transmit CAN frames within Ethernet packets

   Written By Jeremy Daily
   The University of Tulsa

   This program records all received CAN Frames to a buffer, and then transmits the data over ethernet.



*/
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <FlexCAN.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <FastCRC.h>



//Initialize the parameters for Ethernet
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02 };
byte ip[] = { 192, 168, 1, 2 }; // Assigned by DHCP. This may change. This IP address is for a single Source Address
byte server[] = { 192, 168, 1, 4 }; // Python Server IP
uint16_t port = 59581;

// Define J1939 Parameters
#define SOURCE_ADDRESS 0x0B //Brake Controller
#define SOURCE_NAME "Electronic Brake Controller"


//Get access to a hardware based CRC32 
FastCRC32 CRC32;

#define HEARTBEAT_MESSAGE_PERIOD_MS 5000
#define CAN_SEND_TIMEOUT_MS 2000

#define CAN_FRAME_SIZE 25
#define MAX_MESSAGES 19
#define BUFFER_POSITION_LIMIT (CAN_FRAME_SIZE * MAX_MESSAGES)
#define BUFFER_SIZE 512
uint8_t data_buffer[BUFFER_SIZE];
uint16_t current_position;
uint8_t current_channel;
uint8_t message_count = 0;

byte EthBuffer[1024];

elapsedMillis heartbeat_timer;
elapsedMillis CAN_send_timer;
uint32_t heartbeat_counter;

elapsedMicros microsecondsPerSecond;

//Create a counter and timer to keep track of received message traffic
#define RX_TIME_OUT 1000 //milliseconds
elapsedMillis RXTimer;
elapsedMillis lastCAN0messageTimer;
elapsedMillis lastCAN1messageTimer;

const int PWM1Pin = 20;
const int PWM2Pin = 21;
const int PWM3Pin = 22;
const int PWM4Pin = 29;
const int DAC0Pin = A21;
const int DAC1Pin = A22;



EthernetUDP Udp;

int location = 0;

#define PACKET_MAX_SIZE 1400
// buffers for receiving and sending data
char packetBuffer[PACKET_MAX_SIZE];  // buffer to hold incoming packet,

char current_file_name[13];
char prefix[5];

//Create a counter to keep track of message traffic
uint32_t RXCount0 = 0;
uint32_t RXCount1 = 0;

//Define message structure from FlexCAN library
static CAN_message_t rxmsg;
static CAN_message_t txmsg;

uint32_t rxId;
uint8_t len;
uint8_t rxBuf[8];
uint8_t ext_flag;

uint32_t SPN;
float value;

boolean LED_state;

char seperators[] = " ,\t\n";
int ret_val;
char* token;

float get_frequency_from_speed(float km_per_hour, int tone_teeth){
  float revs_per_km = 500*1.6;
  float pulses_per_km = revs_per_km * tone_teeth;
  float km_per_sec = km_per_hour / 3600.00;
  float pulses_per_sec = km_per_sec * pulses_per_km;
  Serial.printf("Pulses per second = %0.3f\n",pulses_per_sec); 
  return pulses_per_sec;
}

/*
 * Function: set_outputs()
 * Requires the packet_size to know how many characters to parse.
 * The format of the message to parse is a comma separated values list.
 * The following data are in groups of 2 (with a source address implied)
 * SPN1,Value1,SPN2,Value2, ... ,SPNn,Valuen
 * 
 * This function is programmed uniquely for each node.
 */
void set_outputs(uint16_t packet_size){
  token = strtok (packetBuffer, seperators); // Get the first SPN
  while (token != NULL)
  {
      sscanf (token, "%lu", &SPN); //Assign the SPN as an integer
      token = strtok (NULL, seperators); // Get the value
      if (token != NULL) ret_val = sscanf (token, "%f", &value); // Get the value as a float
      else break;
      if (ret_val > 0){ // We should have a successful SPN and value pair
        Serial.printf("SPN %lu, Value: %0.3f\n",SPN,value);
        switch (SPN){
          case 84: // Wheel-based vehicle speed
            break;
          case 1592: // Front Axle, Left Wheel Speed
            analogWriteFrequency(PWM1Pin, get_frequency_from_speed(value,100));
            analogWrite(PWM1Pin,127); // 50% duty cycle
            break;
          case 1593: // Front axle, right wheel speed
            analogWriteFrequency(PWM2Pin, get_frequency_from_speed(value,100));
            analogWrite(PWM2Pin,127); // 50% duty cycle
            break;
          case 1594: // Rear axle, left wheel speed
            analogWriteFrequency(PWM3Pin, get_frequency_from_speed(value,100));
            analogWrite(PWM3Pin,127); // 50% duty cycle
            break;
          case 1595: // Rear axle, right wheel speed
            analogWriteFrequency(PWM4Pin, get_frequency_from_speed(value,100));
            analogWrite(PWM4Pin,127); // 50% duty cycle
            break;
          default:
            break;

        }
      }
      token = strtok (NULL, seperators); // Get the next SPN
  }
}

void load_buffer(){
  // reset the timer
  RXTimer = 0;
   
  data_buffer[current_position] = current_channel;
  current_position += 1;
  
  time_t timeStamp = now();
  memcpy(&data_buffer[current_position], &timeStamp, 4);
  current_position += 4;
  
  memcpy(&data_buffer[current_position], &rxmsg.micros, 4);
  current_position += 4;

  memcpy(&data_buffer[current_position], &rxmsg.id, 4);
  current_position += 4;

  // Store the message length as the most significant byte and use the 
  // lower 24 bits to store the microsecond counter for each second.
  uint32_t DLC = (rxmsg.len << 24) | (0x00FFFFFF & uint32_t(microsecondsPerSecond));
  memcpy(&data_buffer[current_position], &DLC, 4);
  current_position += 4;

  memcpy(&data_buffer[current_position], &rxmsg.buf, 8); 
  current_position += 8;

  
  // Check the current position and see if the buffer needs to be written 
  // to the UDP Socket
  check_buffer();
}

void check_buffer(){
  //Check to see if there is anymore room in the buffer
  if (current_position >= BUFFER_POSITION_LIMIT){ //max number of messages
    uint32_t start_micros = micros();
    
    // Write the beginning of each line in the 512 byte block
    sprintf(prefix,"CAN2");
    memcpy(&data_buffer[0], &prefix, 4);
    current_position = 4;
    
    memcpy(&data_buffer[479], &RXCount0, 4);
    memcpy(&data_buffer[483], &RXCount1, 4);
        
    data_buffer[491] = Can0.readREC();
    data_buffer[492] = Can1.readREC();
    //data_buffer[493] = uint8_t(Can2.errorCountRX());
    data_buffer[493] = 0;
    
    data_buffer[494] = Can0.readTEC();
    data_buffer[495] = Can1.readTEC();
    //data_buffer[496] = uint8_t(Can2.errorCountTX());
    data_buffer[496] = 0;
    
    // Write the filename to each line in the 512 byte block
    memcpy(&data_buffer[497], &current_file_name, 8);
  
    uint32_t checksum = CRC32.crc32(data_buffer, 508);
    memcpy(&data_buffer[508], &checksum, 4);
    
    Udp.beginPacket(server, port);
    Udp.write(data_buffer, BUFFER_SIZE);
    Udp.endPacket();
  
    //Reset the record
    memset(&data_buffer,0xFF,512);
    
    //Record write times for the previous frame, since the existing frame was just written
    uint32_t elapsed_micros = micros() - start_micros;
    memcpy(&data_buffer[505], &elapsed_micros, 3);
 
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
  }
}


//Arduino Setup
void setup()
{
  // Start of setup. Code that only runs once
  Serial.println("This device is programmed to work with the following controller application:");
  Serial.println(SOURCE_NAME);
  Serial.printf("J1939 Source address: %d\n\n",SOURCE_ADDRESS);
  
  //PWM Setup Code:
  pinMode(PWM1Pin, OUTPUT);
  pinMode(PWM2Pin, OUTPUT);
  pinMode(PWM3Pin, OUTPUT);
  pinMode(PWM4Pin, OUTPUT);
  pinMode(DAC0Pin, OUTPUT);
  analogWrite(DAC0Pin, 127); // Mid range of 12-bit resolution
  pinMode(DAC1Pin, OUTPUT);
  analogWrite(DAC1Pin, 127); // Mid range of 12-bit resolution

  //LED Code for visualization
  pinMode(LED_BUILTIN, OUTPUT);
  LED_state = true;
  digitalWrite(LED_BUILTIN, LED_state);

  Serial.println("Starting Ethernet.");

  //Ethernet Code: This is sitting in setup, but I'm thinking about making it a function and including it in the loop so that reconnection is possible. Makes the code more robust
  Ethernet.begin(mac, ip);
  Serial.printf("My MAC Address is: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  Serial.printf("My IP Address is:     %03d.%03d.%03d.%03d\n",ip[0],ip[1],ip[2],ip[3]);
  Serial.printf("Server IP Address is: %03d.%03d.%03d.%03d\n",server[0],server[1],server[2],server[3]);
  Serial.println("These IP adresses may need to be reprogrammed to match the current system.");
  Serial.println("Check network connections and ensure the server script is running if no data is scrolling.");
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware.");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(5000);
  }

  // start UDP
  Serial.printf("Starting UDP on port %d", port);
  Udp.begin(port);
  Serial.println("...done.");
  Serial.printf("UDP Max Packet size: %d\n",PACKET_MAX_SIZE);

  //Initialize the CAN channels with autobaud setting
  Can0.begin(0);
  Serial.printf("CAN0 Started at %d bps\n",Can0.baud_rate);
#if defined(__MK66FX1M0__)
  Can1.begin(0);
  Serial.printf("CAN1 Started at %d bps\n",Can1.baud_rate);
#endif
}

/*
 * Reset the microsecond counter and return the realtime clock
 * This function requires the sync interval to be exactly 1 second.
 */
time_t getTeensy3Time(){
  microsecondsPerSecond = 0;
  return Teensy3Clock.get();
}

/*
   The heartbeat signal is a simple test to show there is basic connectivity.
   This should be transmitted infreqently
*/
void send_heartbeat() {
  char message[] = "I'm not Dead ";
  heartbeat_timer = 0;
  byte n = sizeof(message);
  EthBuffer[0] = n;
  memcpy(&EthBuffer[1], &message[0], n);
  heartbeat_counter++;
  memcpy(&EthBuffer[n + 1], &heartbeat_counter, 4);
  uint32_t heartbeat_millis = millis();
  memcpy(&EthBuffer[n + 5], &heartbeat_millis, 4);
  Udp.beginPacket(server, port);
  Udp.write(EthBuffer, n + 9);
  Udp.endPacket();
}


void loop()
{
  //Uncomment to add a heartbeat signal
  //if (heartbeat_timer > HEARTBEAT_MESSAGE_PERIOD_MS) send_heartbeat();

  // Send what's left in the buffer 
  if (CAN_send_timer > CAN_SEND_TIMEOUT_MS){
    CAN_send_timer = 0;
    sprintf(prefix,"TIME"); // Show there is a different timeout frame
    memcpy(&data_buffer[0], &prefix, 4);
    current_position = 4; // Put the position in the right place for the next frame
    
    // Add integrity to the last line of the file.
    uint32_t checksum = CRC32.crc32(data_buffer, 508);
    memcpy(&data_buffer[508], &checksum, 4);
    Udp.beginPacket(server, port);
    Udp.write(data_buffer, BUFFER_SIZE);
    Udp.endPacket();
    //Reset the record
    memset(&data_buffer,0xFF,512);
  }

  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i = 0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, packetSize);
    Serial.print("Contents: ");
    Serial.println(packetBuffer);
    // Reset the buffer
    set_outputs(packetSize);
    memset(packetBuffer, 0x00, packetSize);
  }

  if (Can0.read(rxmsg)) {
    //Serial.printf("%08X\n", rxmsg.id);
    RXCount0++;
    current_channel = 0;
    load_buffer();
  }

#if defined(__MK66FX1M0__)
  if (Can1.read(rxmsg)) {
    RXCount1++;
    current_channel = 1;
    load_buffer();
  }
#endif
}
