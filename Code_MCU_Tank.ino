#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>
#define TRIGGER 14
#define ECHO    12

#define TankContain 34
#define highTank 30
#define lowTank  20

int InputTank = 0;
int val = 1;
int status = WL_IDLE_STATUS;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
const char* ssid = "Es";  //  your network SSID (name)
const char* pass = "wa1234567890";       // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets
//unsigned int ReplyPort = 2390;
IPAddress ReplyIP(192,168,4,255);
char ReplyBuffer[10] = "O";
char packetBuffer[10]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp_r;      // Receive 
WiFiUDP Udp_Send;
//WiFiUDP Udp_s;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200); 
  lcd.begin();
  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  // setting up Station AP
  WiFi.begin(ssid, pass);  
  printWifiStatus(); 
}

void loop()
{
  clearReplyBuffer();
  InputTank = distan();
  Serial.print("Distance :  ");
  Serial.println(InputTank);
  readpack();  
  if(InputTank < highTank){
    delay(5000);
    fncUdpSend();    
  }//------------------------------------------------------
  else {
    delay(15000);
    fncUdpSend();    
  }//------------------------------------------------------
}

void printWifiStatus(){
    // Wait for connect to AP
  Serial.print("[Connecting]");
  Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  int tries=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 0);
    lcd.print("CONNECTING...");
    tries++;
    if (tries > 200){
      Serial.print("No Connected");
      lcd.setCursor(0, 0);
      lcd.print("NO CONNECTED");
      break;
    }    
  }
  if (tries <= 200){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CONNECTED");
    }    
  
  Serial.println();
  IPAddress ip(192,168,4,10);        // static IP 
  IPAddress gateway(192,168,4,12);
  IPAddress subnet(255,255,255,0);
  WiFi.config(ip,gateway,subnet);      // set a static ip to the WiFi 
  
  Serial.println("Connected to wifi");
  Serial.print("started at port ");
  Serial.print(WiFi.localIP());
  Serial.print(" : ");
  Serial.println(localPort);
  Udp_r.begin(localPort);
 
}

int distan(){
   long duration, distance;  
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2);   
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);   
  digitalWrite(TRIGGER, LOW);  
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;
  distance = TankContain - distance;
  distance = (distance*100)/TankContain;
  lcd.setCursor(0, 1);
  lcd.print("TANK = ");  
  if(distance<100) {
    lcd.setCursor(7, 1);
    lcd.print("       ");
  }
  lcd.setCursor(7, 1);
  lcd.print(distance);
  lcd.setCursor(11, 1);
  lcd.print("%");
  return distance;
}

void readpack(){
  clearPacketBuffer();
  int noBytes = Udp_r.parsePacket();
  if ( noBytes ) {
    //Serial.print(millis() / 1000);
    //Serial.print(":Packet of ");
    //Serial.print(noBytes);
    //Serial.print(" received from ");
    //Serial.print(Udp_r.remoteIP());
    ReplyIP = Udp_r.remoteIP(); //---------------------1
    //Serial.print(":");
    //Serial.println(Udp_r.remotePort());
    // We've received a packet, read the data from it
    Udp_r.read(packetBuffer,noBytes); // read the packet into the buffer
    Serial.print("Receive : ");
    // display the packet contents
    for (int i=1;i<=noBytes;i++){
      Serial.print(packetBuffer[i-1]);
      if (i % 32 == 0){
        Serial.println();
      }
      else Serial.print(' ');
    } // end for
    Serial.println();
    val++;
    if(val % 2){
      digitalWrite(2, HIGH);
    }
    else 
    digitalWrite(2, LOW);
    delay(1000);
    //Udp_r.flush();
   //fncUdpSend();
  } // end if
}

void fncUdpSend()
{
  int i = InputTank;
  String s;
  s+= i;
  s.toCharArray(ReplyBuffer,10); 
  Serial.print("Send : ");
  Serial.println(ReplyBuffer);
  Udp_Send.beginPacket(ReplyIP, localPort);
  Udp_Send.write(ReplyBuffer, sizeof(ReplyBuffer));
  Udp_Send.endPacket();
}

void clearPacketBuffer() {
       if(packetBuffer) {
         for(int i = 0; i<sizeof(packetBuffer); i++){
          packetBuffer[i] = NULL;
}     }   }

void clearReplyBuffer() {
       if(ReplyBuffer) {
         for(int i = 0; i<sizeof(ReplyBuffer); i++){
          ReplyBuffer[i] = NULL;
}     }   }      

void tryPing(){  
  fncUdpSend();
  delay(3000);
  readpack();
  if(packetBuffer[0] == 'K'){
     Serial.println("Receive : " + packetBuffer[0]);
  }
}

