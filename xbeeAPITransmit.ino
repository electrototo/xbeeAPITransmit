/* This is my first try in sending a Transmit status packet, via API */ 
//Branch test

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

String payload, incommingData;

void setup() {
  Serial.begin(9600);
  xbee.begin(9600);

  if(! tempsensor.begin()){
    Serial.println("Temp sesor not initialized!");
    while(1);
  }

  Serial.println("[+] Arduino and xbee init...");
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
  byte actualchks;
  
  for(int i = 0; i < p_size; i++){
    xbee.write(packet[i]);
  }

  actualchks = checksum(packet, p_size);
  
  xbee.write(actualchks);
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
    }
      break;
      
    default:
      Serial.println("No packet was transmitted");
  }
  
}

String status_read(){
  // Returns in an array frame ID and delivery status
  //Gota change string answer with byte answer... To saver computer processing power... 
  //Gotta map bytes to failures
  
  String answer;
  byte checksum;

  byte lsb, ftype;
  byte frameID, source16[2], source64[8];
  byte rOptions;

  while(xbee.available()){
    if(xbee.read() == 0x7E){
      discardByte = xbee.read();
      
      lsb = xbee.read();
      ftype = xbee.read();

      if(ftype == 0x8B){
        frameID = xbee.read();

        for(int i = 0; i < 2; i++){
          source16[i] = xbee.read();
        }
        
        byte TRetry = xbee.read();
        byte DStatus = xbee.read();
        byte DiStatus = xbee.read();

        if(DStatus != 0x00){
          answer = "tfailure";
        } 
        else {
          answer = "success";
        }
      } 
      else if (ftype == 0x90){            //Zigbee Receive 
        byte plength = lsb-12;          //Payload length
        byte pText[plength]; 

        for(int i = 0; i < 8; i++){       //Obtain 64-bit address of sender
          source64[i] = xbee.read();
        }

        for(int i = 0; i < 2; i++){       //Obtain 16-bit addres of sender
          source16[i] = xbee.read();
        }
        
        rOptions = xbee.read();

        for(int i = 0; i < (lsb - 14); i++){
          //pText[i] = xbee.read();
          
          Serial.print(xbee.read());
        }

        xbee.read();                      //Checksum

        answer = "transmit";
        
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
  incommingData = status_read();

  if(incommingData == "tfailure"){
    //insert code to retry for certain time... 
  }
  if(incommingData == "transmit"){
    //insert transmition code here...
    Serial.println("[+] Transmitting...");
    payload = "";
  
    temp = tempsensor.readTemperature();
    humidity = tempsensor.readHumidity();

    payload.concat(temp);
    payload.concat(":");
    payload.concat(humidity);
  
    transmit(payload);
    
  }

  delay(100);
}



