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

float temp, humidity;
byte discardByte, msb, lsb, ftype;

int frameID, DStatus;

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

void write_packet(byte packet[], int p_size){
  for(int i = 0; i < p_size; i++){
    xbee.write(packet[i]);
  }
}

void transmit(String payload, byte frame_type=FTYPE){
  xbee.write(SDL);

  switch(frame_type){
    case 0x10:                                        //Transmit 
    {
      int hlen = 14;
      byte fpacket[p_length(14,payload)];
      int ifpacket = 0;

      xbee.write((byte)MSB);
      xbee.write(p_length(14, payload));
    
      fpacket[ifpacket] = FTYPE;
      ifpacket += 1;
      
      fpacket[ifpacket] = 0x01; //Frame ID, gotta change this later
      ifpacket += 1;
     
      for(int i = 0; i < 8; i++){
        fpacket[ifpacket] = 0x00;
        ifpacket += 1;
      }
    
      fpacket[ifpacket] = 0xFF;
      ifpacket += 1;
    
      fpacket[ifpacket] = 0xFE;
      ifpacket += 1;
      
      fpacket[ifpacket] = RADIUS;
      ifpacket += 1;
    
      fpacket[ifpacket] = OPTIONS;
      ifpacket += 1;
    
      for(int i = 0; i < payload.length(); i++){
        fpacket[ifpacket] = (char)payload[i];
        ifpacket += 1;
      }

      write_packet(fpacket, sizeof(fpacket));
    
      xbee.write(checksum(fpacket, sizeof(fpacket)));
    }
      break;
      
    default:
      Serial.println("No packte was transmitted");
  }
  
}

String transmit_status_read(){
  // Returns in an array frame ID and delivery status
  String answer;

  while(xbee.available()){
    if(xbee.read() == 0x7E){
      discardByte = xbee.read();
      
      byte lsb = xbee.read();
      byte ftype = xbee.read();
      byte checksum;

      if(ftype == 0x8B){
        byte frameID;
        byte DStatus;
        
        for(int i = 0; i < lsb-1; i++){
          switch(i){
            case 0:
              frameID = xbee.read();
              break;
            case 4:
              DStatus = xbee.read();
              break;
            default:
              xbee.read();
          }
        }
        
        answer = "Frame ID: ";
        answer.concat(frameID);
        answer.concat(", Delivery Status: ");

        switch(DStatus){
          case 0x00:
            answer.concat("Success!");
            break;
          default:
            answer.concat("Unknown error (gotta change this later)");
        }
        
      } else {
        for(int i = 0; i < lsb-1; i++){
          Serial.print(xbee.read());
        }
      }
      
      checksum = xbee.read();
    }
  }

  return answer;
}

void loop() {
  String payload = "{0:";
  
  temp = tempsensor.readTemperature();
  humidity = tempsensor.readHumidity();

  payload.concat(temp);
  payload.concat(", 1:");
  payload.concat(humidity);
  payload.concat("}");
  
  transmit(payload);

  delay(100);

  Serial.println(transmit_status_read());

  delay(4900);
}



