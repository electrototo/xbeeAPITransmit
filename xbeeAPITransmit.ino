/* This is my first try in sending a Transmit status packet, via API */ 

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_AM2315.h>

SoftwareSerial xbee(2, 3);

#define SDL       0x7E
#define MSB       0x00
#define FTYPE     0x10
#define RADIUS    0x00
#define OPTIONS   0x00 

Adafruit_AM2315 tempsensor;

void setup() {
  Serial.begin(9600);
  xbee.begin(9600);

  if(! tempsensor.begin()){
    Serial.println("Temp sesor not initialized!");
    while(1);
  }
}

byte checksum(byte chArray[], int sizeArray){
  byte total = 0; 
  
  for(int i = 0; i < sizeArray; i++){
    total += chArray[i];
  }

  byte btotal = 0xFF - (total & 0xFF);
  return btotal;
}

byte p_length(int hsize, String payload){
  byte total = 0;

  int plen = hsize + payload.length();
  if(plen > 255){
    //Packet way too big
    return -1;
  } else {
    return (byte)plen; 
  }
}

void transmit(String payload){
  int hlen = 14;
  byte fpacket[p_length(14,payload)];
  int ifpacket = 0;
  
  xbee.write(SDL);
  xbee.write((byte)MSB);
  xbee.write(p_length(14, payload));

  
  xbee.write(FTYPE);
  fpacket[ifpacket] = FTYPE;
  ifpacket += 1;
  
  xbee.write(0x01); //Frame ID, gotta change this later
  
  fpacket[ifpacket] = 0x01;
  ifpacket += 1;
 
  for(int i = 0; i < 8; i++){
    xbee.write((byte)0x00);
    fpacket[ifpacket] = 0x00;
    ifpacket += 1;
  }

  xbee.write(0xFF);
  
  fpacket[ifpacket] = 0xFF;
  ifpacket += 1;
  
  xbee.write(0xFE);

  fpacket[ifpacket] = 0xFE;
  ifpacket += 1;

  xbee.write((byte)RADIUS);
  
  fpacket[ifpacket] = RADIUS;
  ifpacket += 1;
  
  xbee.write((byte)OPTIONS);

  fpacket[ifpacket] = OPTIONS;
  ifpacket += 1;

  for(int i = 0; i < payload.length(); i++){
    xbee.write((char)payload[i]);

    fpacket[ifpacket] = (char)payload[i];
    ifpacket += 1;
  }

  for(int i = 0; i<sizeof(fpacket); i++){
    Serial.print(fpacket[i], HEX);
  }
  Serial.println();

  xbee.write(checksum(fpacket, sizeof(fpacket)));
}

float temp, humidity;


void loop() {
  String payload = "{0:";
  
  temp = tempsensor.readTemperature();
  humidity = tempsensor.readHumidity();

  payload.concat(temp);
  payload.concat(", 1:");
  payload.concat(humidity);
  payload.concat("}");
  
  transmit(payload);
  delay(500);
}



