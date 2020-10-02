//ESP Relay Web Page by Elac 1/1/18

/*-----------------------------------------------------------------------------------------------------
  Access Point                        Web Server  e.g.
  http://192.168.4.1                  http://192.168.1.252 (depends on your network)


  Works with generic ESP01

  Usage:  ACCESS Point:
         After upload search with your device (Phone, Tablet, etc.) for
         new WiFi. Select ESP_FastLED_Access_Point.
         Open in your webbrowser the URL 192.168.4.1
         Optional print a QR-Code with the URL on your lamp http://goqr.me/

         WEB SERVER:
         After upload open the Serial Monitor in Arduino and see what
         IP address is returned. In my case it is 192.168.1.252
         Open this IP address in a browser (PC or phone)

  From Gyro Gearloose J. Bruegl, Feb 2016

  /*------------------------------------------------------------------------------------------------------
  HTTP 1.1 Webserver for ESP8266 adapted to Arduino IDE

  From Stefan Thesen 04/2015
  https://blog.thesen.eu/http-1-1-webserver-fuer-esp8266-als-accesspoint/
  https://blog.thesen.eu/stabiler-http-1-1-wlan-webserver-mit-dem-esp8266-microcontroller
  -----------------------------------------------------------------------------------------------------*/

//edited by Behnam Zakizadeh @ 2020-09-08 (avr64.com)
// add setting page, save latest Relay state
// Based on free src

  
#include <ESP8266WiFi.h>
#include <EEPROM.h>

struct sdata {
  String sMode;
  String sAPssid;
  String sAPpass;
  String sMssid;
  String sMpass;
  String sDevIP;
  String sRelayState;
};

sdata storedData;

byte ReleaseKey = 1;

byte crm; // Holds if command was recieved to change relay mode
byte  relayMode; // Holds current relay state
byte relON[] = {0xA0, 0x01, 0x01, 0xA2};  // Hex command to send to serial for open relay
byte relOFF[] = {0xA0, 0x01, 0x00, 0xA1}; // Hex command to send to serial for close relay

// Select EITHER ACCESS-Point  OR  WEB SERVER setup

// ACCESS-Point setup ------------------------------------------------------------------------------------------------------
const char* ssid = "Wi-Fi Relay";
const char* password = "";  // set to "" for open access point w/o password; or any other pw (min length = 8 characters)

const int relayPin = 0;
const int keyPin = 2;

unsigned long ulReqcount;
unsigned long ulReconncount;
// Create an instance of the server on Port 80
WiFiServer server(80);
//IPAddress apIP(192, 168, 10, 1);   // if you want to configure another IP address
void setup()
{
  pinMode(relayPin, OUTPUT); //0 
  digitalWrite(relayPin, LOW);
  EEPROM.begin(4096);
 
  // Start Serial
  Serial.begin(9600);
  delay(500);

    pinMode(keyPin,OUTPUT); //default Key
    digitalWrite(keyPin,LOW); //turn on Blue LED on Board
    delay(1000);
    digitalWrite(keyPin,HIGH); // off
    pinMode(keyPin,INPUT); //default Key
    //delay(1000);
    Serial.println("Press key...");

    /// default
    if(digitalRead(keyPin)==LOW) {
      Serial.println("Default Key Pressed!");
      //blink LED
      storedData = {
        "AP0",
        "WiFi-Remote",
        "12345678",
        "yourModemSSID",
        "yourModemPass",
        "192.168.1.62", 
        "0"
      };

      save_e(storedData.sMode,0);
      save_e(storedData.sAPssid,10);
      save_e(storedData.sAPpass,50);
      save_e(storedData.sMssid,100);
      save_e(storedData.sMpass,150);
      save_e(storedData.sDevIP,200);
      save_e(storedData.sRelayState,230); 
      
      
      delay(2000);
      digitalWrite(relayPin,0);
      digitalWrite(relayPin,1);
      delay(1000);
      digitalWrite(relayPin,0);
      
      delay(6000);
    }

  
   Serial.println(get_e(0)); 
   Serial.println(get_e(10)); 
   Serial.println(get_e(50)); 
   Serial.println(get_e(100)); 
   Serial.println(get_e(150)); 
   Serial.println(get_e(200)); 
   Serial.println(get_e(230)); 

  storedData.sMode = get_e(0);
  storedData.sAPssid = get_e(10);
  storedData.sAPpass = get_e(50); 
  storedData.sMssid = get_e(100);
  storedData.sMpass = get_e(150);
  storedData.sDevIP = get_e(200);
  storedData.sRelayState = get_e(230);
  digitalWrite(relayPin, storedData.sRelayState.toInt() );

  // setup globals
  relayMode = 0;
  crm = 0;
  ulReqcount = 0;
  ulReconncount = 0;

  // AP mode

  if(storedData.sMode.equals("AP0")){
    WiFi.mode(WIFI_AP);
       //  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // if you want to configure another IP address
        WiFi.softAP(storedData.sAPssid, storedData.sAPpass);
        server.begin();
  }else{
      //IPAddress ip(192, 168, 1, 190); // Put the static IP addres you want to assign here
      IPAddress ip;
      ip.fromString(storedData.sDevIP);
      IPAddress gateway(192, 168, 1, 1); // Put your router gateway here
      IPAddress subnet(255, 255, 255, 0); 
      WiFi.config(ip, gateway, subnet);
      WiFi.mode(WIFI_STA);
      WiFiStart();
  }

}

void WiFiStart()
{
WiFi.begin(storedData.sMssid, storedData.sMpass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // Start the server
  server.begin();
}
/////////////////////////////////////////////////////////////
void loop() {
  Webserver();
  CheckForChange();
  ManualRelay();
 };
///////////////////////////////////////////////////////////
void save_e(String s, int addr){
  //save storedData.sDevIP
  byte len = s.length();
  //Serial.println("len:");
  //Serial.println(len);
  
  byte len_msb = len / 10;
  byte len_lsb = len % 10;
  
  EEPROM.write(addr,len_msb);
  EEPROM.commit();
  
  EEPROM.write(addr+1,len_lsb);
  EEPROM.commit();
  
  int j = 0;
  for(int i=addr+2; i<addr+2+len; i++){
    //Serial.println(s.charAt(j));
    EEPROM.write(i,s.charAt(j));
    EEPROM.commit();
    j++;
  }
}
///////////////////////////////////////////////////////////
String get_e(int addr){
    String result="";
    byte len_msb = EEPROM.read(addr);
    byte len_lsb = EEPROM.read(addr+1);

    byte len = (len_msb*10)+len_lsb; 

    //Serial.println("len read:");
    //Serial.println(len);
    //int j = 0;
    for(int i=addr+2; i<addr+2+len; i++){
       //Serial.print( char(EEPROM.read(i)) );
       //cAPssid[j]=char(EEPROM.read(i));
       //j++;
       //Serial.print(" ");
       //if(i%10==0) Serial.println();
       result +=char(EEPROM.read(i));
     }
    //Serial.println();
    //Serial.println("print arr  cAPssid:");
    ///Serial.println(cAPssid);

    //Serial.println("print str  result:");
    //Serial.println(result);

    //storedData.sDevIP = devip;

  return result;  
}
///////////////////////////////////////////////////////////
void ManualRelay() {
  if(digitalRead(keyPin)==LOW && ReleaseKey==1){ //if key pressed
    if(storedData.sRelayState.equals("0")){
      On();
      ReleaseKey=0;
    }else{
      Off();
      ReleaseKey=0;
    }
    delay(200);
    //debug

    /*
    Serial.println(storedData.sMode);
    Serial.println(storedData.sAPssid);
    Serial.println(storedData.sAPpass);
    Serial.println(storedData.sMssid);
    Serial.println(storedData.sMpass);
    Serial.println(storedData.sDevIP);
    Serial.println(storedData.bRelayState);
*/
    
  }
  if(digitalRead(keyPin)==HIGH){ // key released
    ReleaseKey=1;
  }
};
///////////////////////////////////////////////////////////
void CheckForChange() {
  if (crm == 1)
  {
    switch (relayMode) {
      case 0: {
          Off();
        }
        break;
      case 1: {
          On();
        }
        break;
      case 2: {
          Pulse();
        }
        break;
    }
    crm = 0;
  }
};
///////////////////////////////////////////////////////////////////////////
void Webserver() {   /// complete web server (same for access point) ////////////////////////////////////////////////////////
  String sGetParam = "";

  String saved = "";
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client)
  {
    return;
  }
  // Wait until the client sends some data
  unsigned long ultimeout = millis() + 25; //
  while (!client.available() && (millis() < ultimeout) )
  {
    delay(1);
  }
  if (millis() > ultimeout)
  {
    return;
  }
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  client.flush();
  // stop client, if request is empty
  if (sRequest == "")
  {
    client.stop();
    return;
  }
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath = "", sParam = "", sCmd = "";
  String sGetstart = "GET ";
  int iStart, iEndSpace, iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart >= 0)
  {
    iStart += +sGetstart.length();
    iEndSpace = sRequest.indexOf(" ", iStart);
    iEndQuest = sRequest.indexOf("?", iStart);
    if (iEndSpace > 0)
    {
      if (iEndQuest > 0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart, iEndQuest);
        sParam = sRequest.substring(iEndQuest, iEndSpace);
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart, iEndSpace);
      }
    }
  }
  if (sParam.length() > 0)
  {
    int iEqu = sParam.indexOf("=");
    if (iEqu >= 0)
    {
      sCmd = sParam.substring(iEqu + 1, sParam.length());
      char carray[4];                                // values 0..255 = 3 digits; array = digits + 1
      sCmd.toCharArray(carray, sizeof(carray));      // convert char to the array
    }
  }

  //////////////////////////////
  // format the html response //
  //////////////////////////////
  String sResponse, sHeader;
  ///////////////////////////////
  // 404 for non-matching path //
  ///////////////////////////////
  if (sPath != "/")
  {
    sResponse = "<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    sHeader  = "HTTP/1.1 404 Not found\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  //////////////////////////
  // format the html page //
  //////////////////////////
  else
  {
    ulReqcount++;
    sResponse  = "<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    sResponse += "<script>"; //
      sResponse += "function pageload() {";
      if(storedData.sMode.equals("AP0")){
      //if (strcmp(storedData.sMode,"AP0")==0){
         sResponse += "document.getElementById(\"apdiv\").style.display = \"block\";";
         sResponse += "document.getElementById(\"stadiv\").style.display = \"none\";";  
      }else{
         sResponse += "document.getElementById(\"apdiv\").style.display = \"none\";";
         sResponse += "document.getElementById(\"stadiv\").style.display = \"block\";";
      }
      sResponse += "}"; // for pageload()
      sResponse += "function myFunction() {";
      sResponse += "var x = document.getElementById(\"mode\").value;";
      sResponse += "if(x==\"AP0\"){";
      sResponse += "document.getElementById(\"apdiv\").style.display = \"block\";";
      sResponse += "document.getElementById(\"stadiv\").style.display = \"none\";";
      sResponse += "}else{";
      sResponse += "document.getElementById(\"apdiv\").style.display = \"none\";";
      sResponse += "document.getElementById(\"stadiv\").style.display = \"block\";";
      sResponse += "};";
      sResponse += "}"; // for myFunction()
    sResponse += "</script>";
    sResponse += "<title>Wi-Fi Relay</title></head>";
    sResponse += "<body onload=\"pageload()\" bgcolor=\"#1F1F1F\" >";
    sResponse += "<div style='text-align:center; height:96%; border:10px groove blue; border-radius: 5px;'>";
    sResponse += "<strong><h1><i><a style=color:#00CCCC>Wi-Fi Relay</a></i></h1></strong><br><br><br><br>";
    ///// Buttons
    sResponse += "<form action=\"?sCmd\">";
    if(digitalRead(keyPin)==HIGH){ // if key released display main page
      sResponse += "<button name=\"sCmd\" value=\"ON\" style=\"color:yellow; background-color:black; font-size:130%; width:80px; margin-right:20px;\" onclick=\'this.form.submit();'>ON</button>";
      sResponse += "<button name=\"sCmd\" value=\"OFF\" style=\"color:yellow; background-color:black; font-size:130%; width:80px; margin-left:20px;\" onclick=\'this.form.submit();'>OFF</button>";
      sResponse += "<br /><br /><center><button name=\"sCmd\" value=\"PULSE\" style=\"color:yellow; background-color:black; font-size:130%; width:200px; \" onclick=\'this.form.submit();'>Pulse</button></center>";
    }else{ // if key pressed while press ON, OFF OR PULSE the display setting page
      sResponse += "<center>"; 
      if(storedData.sMode.equals("AP0")){
        sResponse += "<font color=\"white\">Mode: </font><select id=\"mode\" name=\"mode\" onchange=\"myFunction()\"><option value=\"AP0\" selected >AP</option><option value=\"STA\">STA</option></select><br /><br />";
      }else{
        sResponse += "<font color=\"white\">Mode: </font><select id=\"mode\" name=\"mode\" onchange=\"myFunction()\"><option value=\"AP0\" >AP</option><option value=\"STA\" selected >STA</option></select><br /><br />";
      }
      sResponse += "<div id=\"apdiv\"><table>"; 
      sResponse += "<tr><td><font color=\"white\">Board SSID: </font></td><td><input name=\"APssid\" value=\""+storedData.sAPssid+"\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      sResponse += "<tr><td><font color=\"white\">Board Pass: </font></td><td><input name=\"APpass\" value=\""+storedData.sAPpass+"\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      //sResponse += "<tr><td><font color=\"white\">Pulse Second: </font></td><td><input type=\"number\" name=\"plssecap\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      sResponse += "</table></div>"; 
      sResponse += "<div id=\"stadiv\"><table>"; 
      sResponse += "<tr><td><font color=\"white\">Modem SSID: </font></td><td><input name=\"Mssid\" value=\""+storedData.sMssid+"\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      sResponse += "<tr><td><font color=\"white\">Modem Pass: </font></td><td><input name=\"Mpas\" value=\""+storedData.sMpass+"\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      sResponse += "<tr><td><font color=\"white\">Device IP: <small>(Port:80) </small></font></td><td><input name=\"devIP\" value=\""+storedData.sDevIP+"\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      //sResponse += "<tr><td><font color=\"white\">Pulse Second: </font></td><td><input type=\"number\" name=\"plssecsta\" style=\"color:black; background-color:white; font-size:130%; width:200px; \" /></td></tr>";
      sResponse += "</table></div>";
      sResponse += "<br /><br /><center><button name=\"sCmd\" value=\"setting\" style=\"color:yellow; background-color:black; font-size:130%; width:200px; \" onclick=\'this.form.submit();'>Save</button>";
      sResponse += "</center>"; 
    }
    sResponse += "</form><br><br><br><br>";
    /////////////////////////////
    // react on parameters //
    /////////////////////////////
    if (sCmd.length() > 0)
    {
      if (sCmd.indexOf("setting") >= 0)  // process for setting fields
      {
        saved = "Saved! Restart Board to take effect";
        storedData.sMode = sCmd.substring(0,3);
        sGetParam = sCmd.substring(11);
        storedData.sAPssid = sGetParam.substring(0,sGetParam.indexOf('&'));
        sGetParam = sGetParam.substring(sGetParam.indexOf('=')+1);
        storedData.sAPpass = sGetParam.substring(0,sGetParam.indexOf('&'));
        sGetParam = sGetParam.substring(sGetParam.indexOf('=')+1);
        storedData.sMssid = sGetParam.substring(0,sGetParam.indexOf('&'));
        sGetParam = sGetParam.substring(sGetParam.indexOf('=')+1);
        storedData.sMpass = sGetParam.substring(0,sGetParam.indexOf('&'));
        sGetParam = sGetParam.substring(sGetParam.indexOf('=')+1);
        storedData.sDevIP = sGetParam.substring(0,sGetParam.indexOf('&'));

        //save
        save_e(storedData.sMode,0);
        save_e(storedData.sAPssid,10);
        save_e(storedData.sAPpass,50);
        save_e(storedData.sMssid,100);
        save_e(storedData.sMpass,150);
        save_e(storedData.sDevIP,200);
        save_e(storedData.sRelayState,230); 

        //ESP.restart();

      }
      else // process for relay
      {
        // write received command to html page
        // change the effect
        if (sCmd.indexOf("ON") >= 0)
        {
          relayMode = 1;
          storedData.sRelayState = "1";
          crm = 1;
        }
        else if (sCmd.indexOf("OFF") >= 0)
        {
          relayMode = 0;
          storedData.sRelayState = "0";
          crm = 1;
        }
        else if (sCmd.indexOf("PULSE") >= 0)
        {
          relayMode = 2;
          storedData.sRelayState = "0";
          crm = 1;
        }
      }
      

    };

    sResponse += "<br /><a style=color:#00FF00 >";
    sResponse += saved;  
    sResponse += "</a><br />";   
    sResponse += "<font size=+2><a style=color:#FFFDD2 >Relay State:</a><i>&nbsp;";
    
    if(storedData.sRelayState.equals("1")){
      sResponse += "<a style=color:#00FF00 >On</a></i>";
    }else{
      sResponse += "<a style=color:#FF0E0E >Off</a>";
    }
    

    sResponse += "</div></body></html>";
    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
// Send the response to the client
  client.print(sHeader);
  client.print(sResponse);
// and stop the client
  client.stop();
};  // End of web server
///////////////////////////////////////////////////////////////////////////
/// END of complete web server /////////////////////////////////////
///Button functions
void Off() {
  //Serial.write(relOFF, sizeof(relOFF));
  digitalWrite(relayPin, LOW);
  storedData.sRelayState = "0";
  save_e(storedData.sRelayState,230); 
};

void On() {
  //Serial.write(relON, sizeof(relON));
  digitalWrite(relayPin, HIGH);
  storedData.sRelayState = "1";
  save_e(storedData.sRelayState,230); 
};

void Pulse() {
  //Serial.write(relON, sizeof(relON));
  digitalWrite(relayPin, HIGH);
  delay(1000);
  digitalWrite(relayPin, LOW);
  storedData.sRelayState = "0";
  save_e(storedData.sRelayState,230); 
};

///End of sketch/////////////////////////////////////////
