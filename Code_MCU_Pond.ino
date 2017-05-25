#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>

#define DS1307_I2C_ADDRESS 0x68 // the I2C address of Tiny RTC
#define TRIGGER 14
#define ECHO    12
#define RELAY   13

#define PondContain 58
#define lowPond  7


int InputTank = 0;
int InputPond = 0;
int val = 1;
boolean statusSys = false;
boolean statusStopPlay = false;
boolean statusWait = false;     //สถานะที่รอระหว่างน้ำเมื่อเต็มแล้วและเมื่อลดลง เท่ากับ Low tank จะทำงานอีกครั้ง
boolean showOnOff_LCD = false;
boolean refre = false;
boolean bntPlay = false;
boolean btnStop = false;
unsigned char highTank = 30;
unsigned char lowTank = 20;
String co,cmd;

/////////////////////////////about database///////////////////////////
unsigned char countTable = 0,timeCount = 0; 
unsigned char dataTable[27][5];
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
unsigned char timeHoData = 12, timeMinData = 18;
unsigned char pondData = 0, tankData = 0;
boolean timeState = false;

//////////////////////////about connection//////////////////////////////
//WiFiClient client;
LiquidCrystal_I2C lcd(0x3F, 16, 2);
ESP8266WebServer server(80);
int status = WL_IDLE_STATUS;
const char* ssid = "Es";  //  your network SSID (name)
const char* pass = "wa1234567890";       // your network password
unsigned int localPort = 2390;      // local port to listen for UDP packets
char packetBuffer[10]; //buffer to hold incoming and outgoing packets
char ReplyBuffer[10];
boolean StopIfNotSendState = false;
unsigned char StopIfNotSend = 0;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp_Send;
WiFiUDP Udp_Rec;

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */

IPAddress ipMulti(192,168,4,1);
unsigned int portMulti = 23;      // local port to listen on
IPAddress ReplyIP(192,168,4,255);

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte vala)
{
return ( (vala/10*16) + (vala%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte vala){
  return ( (vala/16*10) + (vala%16) );
}

/////////////////////////// Function to set the currnt time/////////////////////////////////
////////////////////change the second&minute&hour to the right time/////////////////////////
void setDateDs1307(){
  second =00;
  minute = 54;
  hour = 14;
  dayOfWeek = 2;
  dayOfMonth =10;
  month =4;
  year= 17;
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decToBcd(0));
  Wire.write(decToBcd(second)); // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour)); // If you want 12 hour am/pm you need to set
  // bit 6 (also need to change readDateDs1307)
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

///////////////////////////Function to gets the date and time ///////////////////////////////
///////////////////////////from the ds1307 and prints result////////////////////////////////
void getdatafunction(){
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(decToBcd(0));
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  second = bcdToDec(Wire.read() & 0x7f);
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0x3f); // Need to change this if 12 hour am/pm
  dayOfWeek = bcdToDec(Wire.read());
  dayOfMonth = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read()); 
   
    Serial.print(hour, DEC);
    Serial.print(":");
    Serial.print(minute, DEC);
    Serial.print(":");
    Serial.print(second, DEC);
    Serial.print(" ");
    Serial.print(month, DEC);
    Serial.print("/");
    Serial.print(dayOfMonth, DEC);
    Serial.print("/");
    Serial.print(year,DEC);
    Serial.print(" ");
    Serial.println();
    //Serial.print("Day of week:");
 //   printTable();
    delay(2000); 
}

////////////////////////////////Save Table////////////////////////////
void saveTable(){
    dataTable[26][1] = hour;
    dataTable[26][2] = minute;
    dataTable[26][3] = InputPond;
    dataTable[26][4] = InputTank;    
    for(unsigned char i=1;i<26;i++){
      dataTable[i][1] = dataTable[i+1][1];
      dataTable[i][2] = dataTable[i+1][2];
      dataTable[i][3] = dataTable[i+1][3];
      dataTable[i][4] = dataTable[i+1][4];
    }
    delay(1000);
}

///////////////////////////////printTable////////////////////////////
void printTable(){    
      Serial.print("NO."); Serial.print("     ");
      Serial.print("Time");Serial.print("         ");      
      Serial.print("Pond");Serial.print("      ");
      Serial.println("Tank");
     for(unsigned char i=1;i<26; i++){
      Serial.print(i);
      Serial.print("        ");
      Serial.print(dataTable[i+1][1]);
      Serial.print(" : ");
      Serial.print(dataTable[i+1][2]);
      Serial.print("       ");
      Serial.print(dataTable[i+1][3]);
      Serial.print("       ");
      Serial.println(dataTable[i+1][4]);
    } 
   Serial.println("");
}

////////////////////////////////////////////Put HTML Last status Screen/////////////////////////////////////////
void handleRoot(){
      cmd = "";
      cmd += "<!DOCTYPE HTML>\r\n";
      cmd += "<html>\r\n";      
      cmd += "<head>";
      cmd += "<meta http-equiv='refresh' content='5'/>";
      cmd += "</head><body>";
      
      cmd +="";
      cmd +="";
      
      cmd +="<table>";
      cmd +="<tr>";
      cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<th>TIME</th>";
      cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
      cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<th>POND</th>";
      cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
      cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<th>TANK</th>";
      cmd +="</tr><br>";
     
      for(unsigned char i=1;i<25; i++){
          cmd +="<table>";
          cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<tr>";
          cmd +="<td>";
          cmd += dataTable[i+1][1];
          cmd += "&nbsp;:&nbsp;";
          cmd += dataTable[i+1][2];
          cmd += "</td>";

          cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
          cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<td>";
          
          cmd += dataTable[i+1][3];
          cmd +="</td>";

          cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
          cmd +="&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<td>";
         
          cmd += dataTable[i+1][4];
          cmd +="</td></tr><br>";          
      } 
      
      cmd +="</table>";
      cmd += "</body>";     
      cmd += "<html>\r\n";
   //   server.send(200, "text/html", cmd);
}

//////////////////////////////////WiFi Status////////////////////////////////
void printWifiStatus(){
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(" AP IP Address: ");
  Serial.print(myIP);
  Serial.println();
  Serial.print("server started at : ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(localPort); 
  //Udp_Rec.beginMulticast(myIP,  ipMulti, portMulti);  //--------------
  Udp_Rec.begin(localPort); 
}
/////////////////////////////////Find distance////////////////////////////
int distan(){
   long duration, distance;
   int di;
  digitalWrite(TRIGGER, LOW);  
  delayMicroseconds(2);   
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);   
  digitalWrite(TRIGGER, LOW);  
  duration = pulseIn(ECHO, HIGH);
  distance = (duration/2) / 29.1;
  distance = PondContain - distance;
  di = (distance*100)/PondContain;
//  Serial.print("Distance : ");
//  Serial.println(di);
  lcd.setCursor(0, 1);
  lcd.print("POND = ");
  lcd.setCursor(7, 1);
  lcd.print(di);
  if(di<100) {
    lcd.setCursor(9, 1);
    lcd.print("    ");
  }
  lcd.setCursor(10, 1);
  lcd.print("%");
  return di;
}
/////////////////////////////////Read Packet From Tank////////////////////////////
void readpack(){
  clearPacketBuffer();
  int noBytes = Udp_Rec.parsePacket();
  if ( noBytes ) {
  //  Serial.print(millis() / 1000);
  //  Serial.print(":Packet of ");
  //  Serial.print(noBytes);
  //  Serial.print(" received from ");
  //  Serial.print(Udp_Rec.remoteIP());
    ReplyIP = Udp_Rec.remoteIP();//-------------------------------2
  //  Serial.print(":");
  //  Serial.println(Udp_Rec.remotePort());
    // We've received a packet, read the data from it
    Udp_Rec.read(packetBuffer, noBytes); // read the packet into the buffer
/*     Serial.print("Receive : ");
    
    // display the packet contents
   for (int i=1;i<=noBytes;i++){
      Serial.print(packetBuffer[i-1]);
      if (i % 32 == 0){
        Serial.println();
      }
      else Serial.print(' ');
    } // end for 
    Serial.println();*/
    val++;
    if(val % 2){
      digitalWrite(2, HIGH);
      lcd.setCursor(0,15);
      lcd.print("O");
    }
    else 
    digitalWrite(2, LOW);
    lcd.setCursor(0,15);
    lcd.print(" ");
    StopIfNotSend = 0;
//    delay(1000);  
    //fncUdpSend();
  } // end if
    int j = atoi(packetBuffer);
    if(j != 0){
      InputTank = j;     
 //     Serial.print("InputTank : ");
 //     Serial.println(InputTank);
    }
    
  //fncUdpSend(); 
}
///////////////////////////Function Send//////////////////////
void fncUdpSend()
{  
 // strcpy(packetBuffer, "HELLO!");
  Serial.print("Send : ");
  Serial.println(ReplyBuffer);
  Udp_Send.beginPacket(ipMulti, localPort);
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

void handleNotFound(){
  digitalWrite(LED_BUILTIN,HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN,LOW);
}

//////////////////////////////////SET  UP///////////////////////////////
void setup(){
  Wire.begin();
  //setDateDs1307(); //Set current time;
  //Open serial communications and wait for port to open:  
  Serial.begin(115200);
  // prepare GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(RELAY, OUTPUT);
  showOnOff_LCD = false;
  lcd.begin();
  WiFi.softAP(ssid, pass);
  printWifiStatus();  
  lcd.print("IP:");
  lcd.setCursor(4, 0);
  lcd.print(WiFi.softAPIP());
  
   if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }  

  //////////////////////////////APP CONTROL//////////////////////////    
//  server.on("/", handleRoot);
  server.on("/OPEN",[](){
      server.send(200, "text/plain","CONNECTED");
 //     server.send(200, "datasend","OK");      
  });


///////////////////////////////Main Screeen////////////////////////  
   server.on("/SYSTEM=AUTO",[](){
      server.send(200, "text/plain","AUTO");
      statusSys = false;
      statusStopPlay = false;
  });
  server.on("/SYSTEM=MANUAL",[](){
      server.send(200, "text/plain","MANUAL");
      statusSys = true;
        });

   server.on("/REFRESHTANK",[](){
       co = String(InputTank);  
       server.send(200, "datasendTank",co);       
    });
  
   server.on("/REFRESHPOND",[](){       
       co = String(InputPond);
       server.send(200, "datasendPond",co);
    });
  server.on("/REFRESHMODE",[](){       
       if(statusSys){
        server.send(200, "text/plain","MANUAL");
      }
      else{
        server.send(200, "text/plain","AUTO");
      }      
    });
  server.on("/REFRESHSYSTEM",[](){       
       if(showOnOff_LCD){
        server.send(200, "PLAYON","PLAYON");
      }
      else{
        server.send(200, "text/plain","STOP");
      }
    });
  
  server.on("/SYSTEMPLAYON",[](){        
        if(statusSys){          
       //   statusStopPlay = true;          
          if(InputPond > lowPond && InputTank < highTank){
            server.send(200, "PLAYON","PLAYON");
            digitalWrite(RELAY, HIGH);
            showOnOff_LCD = true;
 //           Serial.println("  SYSTEM ON");
            delay(2000);
          }
          else{        
            digitalWrite(RELAY, LOW);
            server.send(200, "text/plain","STOP");
            showOnOff_LCD = false;
//            Serial.println("  NO WATER POND OR  WATER TANK FULL");
            delay(2000);
          }}        
  });
  server.on("/SYSTEM=OFF",[](){       
       if(statusSys){
        server.send(200, "text/plain","STOP");
        digitalWrite(RELAY, LOW);
        showOnOff_LCD = false;
//        Serial.println("  SYSTEM OFF");
     //   statusStopPlay = false;
       } 
  });

//////////////////////////////////Last Status Screen/////////////////////////////////////  
   server.on("/GETLASTSTATUS",[](){
      handleRoot();
      server.send(200, "text/html",cmd);
  });

 //////////////////////////////////Set Level////////////////////////////////////////////////
  server.on("/10",[](){
    lowTank = 10;
    server.send(200,"setLow","10");
  });
  server.on("/20",[](){
    lowTank = 20;
    server.send(200,"setLow","20");
  });
  server.on("/30",[](){
    lowTank = 30;
    server.send(200,"setLow","30");
  });
  server.on("/40",[](){
    lowTank = 40;
    server.send(200,"setLow","40");
  });
  server.on("/60",[](){
    highTank = 60;
    server.send(200,"setHigh","60");
  });
  server.on("/70",[](){
    highTank = 70;
    server.send(200,"setHigh","70");
  });
  server.on("/80",[](){
    highTank = 80;
    server.send(200,"setHigh","80");
  });
  server.on("/90",[](){
    highTank = 90;
    server.send(200,"setHigh","90");
  });
  
  server.on("/RefreshSetHigh",[](){
    co = String(highTank);
    server.send(200,"LevelHigh",co);});

   server.on("/RefreshSetLow",[](){
    co = String(lowTank);
    server.send(200,"LevelLow",co);
  }); 
  
  server.on("/BACK",[](){
    server.send(200,"text/plain","BACK");
  });
  
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("server started");
}

/////////////////////////////////////LOOP////////////////////////////////////////////
void loop()
{    
    //resetClient();   
    server.handleClient();    
    clearReplyBuffer();
//    Serial.print("Pond Distance : ");
//    Serial.println(distan());  
    InputPond = distan();    
    readpack();
    getdatafunction();
    server.handleClient();
//    printTable();
    if(!statusSys){
      if(InputTank == 0 || InputPond == 0){
        digitalWrite(RELAY, LOW);
        showOnOff_LCD = false;
        delay(1000);
      }//--------------------------------------------------------
      else if(InputTank > highTank){
        digitalWrite(RELAY, LOW);
        showOnOff_LCD = false;
        statusWait = false;
    //    Serial.println("  SYSTEM OFF WATER TANK FULL");        
        delay(1000);
     }//---------------------------------------------------------
      else if(InputTank < lowTank){
        statusWait = true;
        InputPond = distan();        
        delay(1000);
        if(InputPond > lowPond){
          digitalWrite(RELAY, HIGH);
          showOnOff_LCD = true;
   //       Serial.println("  SYSTEM ON");                    
           }
        else{        
          digitalWrite(RELAY, LOW);
          showOnOff_LCD = false;
   //       Serial.println("  NO WATER POND ");          
        }
      }//---------------------------------------------------------
     
      else if(InputTank < 0){
        statusWait = true;
        InputPond = distan();        
        delay(1000);
        if(InputPond > lowPond){
          digitalWrite(RELAY, HIGH);
          showOnOff_LCD = true;
     //     Serial.println("  SYSTEM ON");                    
           }
        else{        
          digitalWrite(RELAY, LOW);
          showOnOff_LCD = false;
      //    Serial.println("  NO WATER POND ");          
        }
      }//---------------------------------------------------------
      else if(InputTank >= lowTank && InputTank <= highTank){        
        delay(1000);
        if(statusWait){
          InputPond = distan();
          if(InputPond > lowPond){
            digitalWrite(RELAY, HIGH);            
            showOnOff_LCD = true;
            Serial.println("  SYSTEM ON");            
          }
          else{        
            digitalWrite(RELAY, LOW);            
            showOnOff_LCD = false;
            Serial.println("  NO WATER POND ");
            }
        }        
      }//---------------------------------------------------------      
      else{
        digitalWrite(RELAY, LOW);        
        showOnOff_LCD = false;
        delay(1000);
     }
    }
    
   else{
  //    Serial.println("MANUAL MODE");
      if(InputPond < lowPond || InputTank > highTank){                 
            digitalWrite(RELAY, LOW);
            server.send(200, "text/plain","STOP");
            showOnOff_LCD = false;
        //    Serial.println("  NO WATER POND OR  WATER TANK FULL");
            delay(1000);
       }          
      
      delay(1000);
    }
//    Serial.println();
    server.handleClient();
    if(showOnOff_LCD){      
      lcd.setCursor(13, 1);
      lcd.print("   ");
      lcd.setCursor(13, 1);
      lcd.print("ON");
    }
    else{
      lcd.setCursor(13, 1);
      lcd.print("OFF");
    }
    
/*    if(minute == 0){
      timeCount = 60;}    
    else{*/
      timeCount = minute;//}
         
    if(timeCount%2 != 0){
      timeState = true;}
    else{
      if(timeState){ 
        timeState = false;
       // getdatafunction();
        saveTable();
      }
    }

    if(timeCount%2 != 0){
      StopIfNotSendState = true;
    }
    else{
      if(StopIfNotSendState){
        StopIfNotSend++;
        StopIfNotSendState = false;
      }      
    }

    if(StopIfNotSend == 2){
      digitalWrite(RELAY, LOW);
      showOnOff_LCD = false;
      StopIfNotSend =0;
    }
    
    delay(500);
}


