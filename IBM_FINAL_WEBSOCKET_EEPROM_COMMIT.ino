
//#include "ESP8266WiFi.h"
//#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <PubSubClient.h> // https://github.com/knolleary/pubsubclient/releases/tag/v2.3
#include <ArduinoJson.h>
#include <SPI.h>
#include<SD.h>
#include <WiFiUdp.h>
//Serial.begin(9600);
//boolean isconnected;

WebSocketsServer webSocket = WebSocketsServer(80);
const char *apssid = "";
const char *appassword = "";

boolean esp_connected =false;

//ESP8266WebServer server = ESP8266WebServer(80);

String ssid,password;
String response = "received msg:";



unsigned int localPort = 2390;
IPAddress timeServerIP; // in.pool.ntp.org NTP server address
//const char* ntpServerName = "in.pool.ntp.org";
const char* ntpServerName = "in.pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];

WiFiUDP udp;

//FS
#include<FS.h>


//Device info
#define ORG ""
#define DEVICE_TYPE ""
#define DEVICE_ID ""
#define TOKEN ""


//IBM Topics and Authentication token

char server[] = ORG ".messaging.internetofthings.ibmcloud.com";
char authMethod[] = "use-token-auth";
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID;

const char publishTopic[] = "iot-2/evt/status/fmt/json";
const char responseTopic[] = "iotdm-1/response";
const char manageTopic[] = "iotdevice-1/mgmt/manage";
const char updateTopic[] = "iotdm-1/device/update";
const char rebootTopic[] = "iotdm-1/mgmt/initiate/device/reboot";

const char factoryResetTopic[] = "iotdm-1/mgmt/initiate/device/factory_reset";





//IBM
void handleUpdate(byte*);
void initManagedDevice();
void publishData();
void wifiConnect();

void getSerialData();
void readFlash(unsigned int );
void readSD();
void flash_failed_re_init();

const int chipSelect = 4;

int publishInterval = 10000; // 10 seconds
long lastPublishMillis;

unsigned int sdid_flash;
unsigned int sdid_sd;

unsigned long write_at;
unsigned long send_from;
unsigned long file_sent;
unsigned long packet_count;
unsigned long count = 0;



String payload; // "{\"d\":{\"V1\":\"0.0\",\"I1\":\"-1.0\",\"V2\":\"-1.0\",\"I2\":\"-1.0\",\"V3\":\"-1.0\",\"I3\":\"-1.0\",\"V4\":\"-1.0\",\"I4\":\"-1.0\",\"V5\":\"-1.0\",\"I5\":\"-1.0\",\"Phi1\":\"-1.0\",\"Phi2\":\"-1.0\",\"Phi3\":\"-1.0\",\"ExT\":\"-1.0\",\"T1\":\"-1.0\",\"TS\":\"-1\"}}";
String payload1;
String payload2;
WiFiClient wifiClient;

void callback(char* topic, byte* payload, unsigned int payloadLength);
PubSubClient client(server, 1883, callback, wifiClient);
unsigned long sendNTPpacket(IPAddress& address);




//mqtt


//flags

boolean flag_sd_ready = false;
boolean flag_is_online = false;
boolean flag_over_write = false;



















void setup() {
  Serial.begin(9600);
  EEPROM.begin(512);
  //boolean is_connected_to_eeprom_wifi =false ;
  //boolean pass_control_to_main_code= false;
   SPIFFS.begin();
    Serial.println("SPIFFS initialized");

//  Serial.println("Initializing SD card...");
//  if (!SD.begin(chipSelect)) {
//    Serial.println("Card failed, or not present");
//  }
//  else {
    Serial.println("card initialized.");
    flag_sd_ready = true;
    delay(50);
    readSD();
    readFlash(sdid_sd);
 // }
  /*
   *  Retrive ssid and password from EEPROM
   *  Connect to the wifi network with credentials retrieved from EEPROM
   *  If it gets connected publish the data as usual (Control should now go to the code written by tejas
   *  If esp cannot connect to the internet Run it in AP mode to get credentials from user/app 
   *  Get ssid and password from user
   *  {
   *  Clear the eeprom
   *  Store ssid
   *  Store Password
   *  }
   *  Connect to wi-fi  
   */

  /*   Uncomment this layer
    Spi_File_F1=SPIFFS.open("/sampleopen","r");
  




  */
  
  
  
  Spi_File f1 = SPIFFS.open("/info.txt", "r");
  if (f1)
  {
    String s;
    Serial.println("inside info file");
    Serial.print("Start storing backup in this index: ");
    s = f1.readStringUntil('\n');
    //Serial.print(":");
    Serial.println(s);
    write_at = s.toInt();

    Serial.print("Start sending data from this file: ");
    s = f1.readStringUntil('\n');
    //Serial.print(":");
    Serial.println(s);
    send_from = s.toInt();

    Serial.print("no of files sent: ");
    s = f1.readStringUntil('\n');
    //Serial.print(":");
    Serial.println(s);
    file_sent = s.toInt();

    f1.close();
  }
  else
  {
    Serial.println("info file failed to load");
    Serial.println("Erasing flash. ESP will reboot.");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
    flash_failed_re_init();
  }

  Spi_File f6 = SPIFFS.open("/count.txt", "r");
  if (f6)
  {
    Serial.println("Reading data from count file....");
    String s = f6.readStringUntil('\n');
    Serial.print("packet count read from flash is : ");
    Serial.println(s);
    packet_count = s.toInt();
    f6.close();
  }
  else
  {
    Serial.println("failed to open count file");
  }

    String ssid_retrieved,pass_retrived;

    ssid_retrieved=get_ssid();
    pass_retrived=get_pass();
    testwifi(ssid_retrieved,pass_retrived);
    if (esp_connected==true)
    {
      Serial.println("Publishing will now begin");
    }
    else
    {
      init_websocket();
      get_credents_and_check();
      Serial.println("Publishing will now begin");
      
    }
     
    mqttConnect();
  initManagedDevice();

  //Initialize
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
    }

void init_websocket()
{
  WiFi.softAP(apssid, appassword);
  Serial.println(" ");
  IPAddress myIP = WiFi.softAPIP();
  Serial.println("AP IP address: ");
  Serial.println(myIP);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}




void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len)
{
  Serial.println("request received from browser");
  webSocket.sendTXT(num, "succesfull connection");
  switch (type) {
    case WStype_DISCONNECTED:
      {
        Serial.printf("socket disconnected:[%u]\n", num);
      }
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d  %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;

    case WStype_TEXT:
      {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        // webSocket.sendTXT(num,response);
        Serial.println("text received");
        String text = String((char *) &payload[0]);
        if (text.indexOf('`') > -1)
        {
          int loc = text.indexOf('`');
          Serial.print("location:");
          Serial.println(loc);
          ssid = text.substring(0, loc);
          password = text.substring(loc + 1);
          Serial.print("ssid:");
          Serial.println(ssid);
          Serial.print("password:");
          Serial.println(password);
          WiFi.mode(WIFI_STA);
          delay(500);
          delay(500);
          delay(500);
          char ssid2[40];
          char password2[40];
          delay(500);
          ssid.toCharArray(ssid2, ssid.length() + 1);
          password.toCharArray(password2, password.length() + 1);
          Serial.print("ssid2:");
          Serial.println(ssid2);
          Serial.print("password2:");
          Serial.println(password2);
          //WiFi.begin(ssid2, password2);
          Serial.println("connecting..");
          testwifi(ssid2,password2);
          

        }
        break;
      }

    case WStype_BIN:
      {
        hexdump(payload, len);

        // echo data back to browser
        webSocket.sendBIN(num, payload, len);
      }
      break;
  }
}
    
        

void get_credents_and_check()
{   
    String test1,test2;
    boolean websokcet;
    Serial.println("It seems that you have entered incorrect credentials.Please enter new credentials");
    
    
    // This gets executed When user has entered credentials for the first time or when entered credents are invalid
    //Check if ssid entered is available or not
    //********************If it is not available. work in offline mode instead of asking for credents once again. **********************************
    //If it is available and device gets connected : Store the credents in eeprom
    // If it is available and  device ghets connected but net connection is absent.. : Do not worry,ESP will store the data in esp(Search below to find SD card storage code.
    //If it is available and device cannot connect to it... That means password entered is invalid and start working in ap mode, and retrive credents from user.
    //If wifi scan does not show the ssid Operate in AP mode.  
    
    Serial.println("Enter ssid");
  
    while(!Serial.available()) ;
    test1=Serial.readString();
    //test1=test1.c_str();
    test1.trim();  // string.trim() removes the blank spaces in the string.
    
    Serial.println("Enter password");
  
    while(!Serial.available()) ;
    test2=Serial.readString();
    //test2=test2.c_str();
    test2.trim();
    
    while(esp_connected==false)
    {
      webSocket.loop();
    }
    write_eeprom(ssid,password);
    ESP.restart();
    //testwifi(test1,test2);
    
   
}

String get_ssid()

{



  String esid="";
  Serial.println("Reading EEPROM ssid");
  for (int i = 0; i < 32; i++)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  return esid;
}


String get_pass()
{
  Serial.println("Reading EEPROM pass");
  String epass="" ;
  for (int i = 32; i < 96; i++)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  //if ( esid.length() > 1 ) {
     // test esid 
     // WiFi.begin(esid.c_str(), epass.c_str());
     // if ( testWifi() == 20 ) { 
     // launchWeb(0);
  return epass ;
      
  
  
}


void write_eeprom( String qsid,String qpass)
{


        int c;
        c=qsid.length();
        Serial.println(c);    
        for (int i = 0 ; i <512 ; i++) 
            {
            EEPROM.write(i, 0);                               //Erase everything
            }

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); i++)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); i++)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
}



void testwifi(String esid,String epass)
{ 
  /*
 //WiFi.mode(WIFI_STA);
 WiFi.begin(esid.c_str(), epass.c_str());
//  WiFi.begin(esid, epass);
  
  delay(10000);
   if (WiFi.status() == WL_CONNECTED)
  {
     isconnected=true;
    Serial.println("Congrats! ESP connected to the world! Transmission will now begin");
    
  }



   else
   {
    //WiFi.mode(WIFI_OFF);
    get_credents_and_check();
    ESP.restart();
   
   }      
   */     WiFi.begin(esid.c_str(), epass.c_str());
          unsigned int temp_time = millis();
          while (WiFi.status() != WL_CONNECTED && millis() - temp_time < 15000)
          {
            delay(200);
            Serial.println("..");
            esp_connected = false;
          }
          if (WiFi.status() == WL_CONNECTED) {
            esp_connected = true;
          }
          if (esp_connected)
          {
            Serial.println("");
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
          }
          else
          {
           get_credents_and_check();
          }





}

/*
Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();

*/

void loop() {
  // put your main code here, to run repeatedly:

  if (millis() - lastPublishMillis > publishInterval) {
    publishData();
    lastPublishMillis = millis();
  }
  else {
    delay(50);
  }

  if (!client.loop())
  {
    mqttConnect();
  }
}


void mqttConnect() {
  int start_time = millis();
  if (millis() - start_time < 3000)
  {
    if (!!!client.connected()) {
      Serial.print("Reconnecting MQTT client to "); Serial.println(server);
      while (!!!client.connect(clientId, authMethod, token) && millis() - start_time < 5000) {
        Serial.print(".");
        delay(250);
      }
      Serial.println();
    }

  }
}



void publishData() {
  //  if(SD.exists("11111111.txt"))
  //{
  //  Serial.println("exists");
  //}
  //else
  //{
  //  Serial.println("doesnt exists");
  //}
  getSerialData();
  if (client.publish(publishTopic, (char*) payload.c_str())) {
    Serial.println("Publish OK");
    Serial.println(millis());

    //sending backup data
    Serial.println("Net connection available. Will look for any backlog in microsd card");
    String file_name = "";
    file_name += String(send_from);
    file_name += ".txt";
    if (write_at >= 2 && write_at > send_from )
    {
      Serial.print("send data from file ");
      Serial.println(send_from);

      File backlog = SD.open(file_name);
      if (backlog)
      {
        //read data from file
        Serial.print("Inside file ");
        Serial.println(file_name);
        while (backlog.available()) {
          delay(1);
          String temp = "";
          char data = backlog.read();
          while (data != '\n')
          {
            temp += data;
            data = backlog.read();
          }
          Serial.println("Message read from microsd is:" );
          Serial.println((char*)temp.c_str());

          //send data to server
          if (client.publish(publishTopic, (char*) temp.c_str()))
          {
            Serial.println("Publish OK");
          }
          //if sending fails
          else {
            Serial.println("Seems like net connection is gone again...will try sending afterwards");
            Serial.println("Time being function will return abruptly.");
            backlog.close();
            return;
          }
        }
        backlog.close();
        //open same file again with append
        File same_file = SD.open(file_name, O_WRITE | O_CREAT | O_TRUNC);
        if (same_file)
        {
          Serial.println("Truncated file to zero length.");
          Serial.println(same_file.size());
        }

        //update details in info file
        Spi_File f11 = SPIFFS.open("/info.txt", "w");
        if (f11)
        {
          Serial.println("Updating info file");
          Serial.print("Start storing backup in this index: ");
          Serial.println(String(write_at));
          f11.println(String(write_at));
          write_at = write_at;

          Serial.print("Start sending data from this file: ");
          Serial.println(String(send_from + 1));
          f11.println(String(send_from + 1));
          send_from = send_from + 1;

          Serial.print("no of files sent: ");
          Serial.println(file_sent);
          f11.println(String(file_sent + 1));
          file_sent = file_sent + 1;
          f11.close();
        }
      }
      else {
        Serial.println("File open failed.");
        //update info file content
        Spi_File f11 = SPIFFS.open("/info.txt", "w");
        if (f11)
        {
          Serial.println("Updating info file");
          Serial.print("Start storing backup in this index: ");
          Serial.println(String(write_at));
          f11.println(String(write_at));
          write_at = write_at;

          Serial.print("Start sending data from this file: ");
          Serial.println(String(send_from + 1));
          f11.println(String(send_from + 1));
          send_from = send_from + 1;

          Serial.print("no of files sent: ");
          Serial.println(file_sent);
          f11.println(file_sent);
          //no change in file_sent
          f11.close();

        }
      }

    }
    else {
      Serial.println("Yeah...nothing to send.");
    }
  }
  else {
    flag_is_online = false;
    Serial.println("Publish FAILED");

    //open latest file and start logging data into it


    String file_name2 = String(write_at);
    file_name2 += ".txt";

    Serial.println("Just before file exist function");
    Serial.print(file_name2);

    File f2 = SD.open(file_name2, FILE_WRITE);
    if (f2)
    {
      if (f2.size() < 300)
      {
        Serial.print("saving data on ");
        Serial.println(file_name2);
        f2.println(payload);
        f2.close();
        Serial.println("payload saved on sd card.");
      }
      else {
        Serial.print("file size limit reached for ");
        Serial.println(file_name2);
        //update info file..make new filename and write into that
        SPIFFS.remove("/info.txt");
        Spi_File f3 = SPIFFS.open("/info.txt", "w");
        if (f3)
        {
          Serial.println("Updating info file");
          Serial.print("Start storing backup in this index: ");
          Serial.println(String(write_at + 1));
          f3.println(String(write_at + 1));
          write_at = write_at + 1;

          Serial.print("Start sending data from this file: ");
          Serial.println(String(send_from));
          f3.println(String(send_from));
          //no change in send_from

          Serial.print("no of files sent: ");
          Serial.println(file_sent);
          f3.println(file_sent);
          //no change in file_sent

          f3.close();
          //now write data into new file
          String file_name3 = "";
          file_name3 += String(write_at);
          file_name3 += ".txt";
          File f4 = SD.open(file_name3, FILE_WRITE);
          if (f4)
          {
            f4.println(payload);
            Serial.print("data written into  file");
            Serial.println(file_name3);
            f4.close();
          }
          else {
            Serial.println("failed to open new file");
          }
        }
        else {
          Serial.println("failed to open info file for upgrading");
          Serial.println("ESP will restart");
          delay(500);
          ESP.restart();
        }
      }
    }
    else {
      Serial.println("failed to write into sd card. data lost.");
      Serial.println("ESP will restart");
      delay(500);
      ESP.restart();
    }

    //payload = "{\"d\":{\"V1\":\"";
    //payload += String(random(200, 300)); //String(count);
   // payload += ",\"I1\":\"1.0\",\"V2\":";
   // payload += String(random(200, 300));
    //payload += ",\"I2\":\"1.0\",\"V3\":\"10.0\",\"I3\":\"1.0\",\"V4\":\"120.0\",\"I4\":\"-1.0\",\"V5\":\"-1.0\",\"I5\":\"-1.0\",\"Phi1\":\"-1.0\",\"Phi2\":\"-1.0\",\"Phi3\":\"10.0\",\"ExT\":\"50.0\",\"T1\":\"38.0\",\"TS\":";
    //payload += String(-1);
    //payload += "}}";
  }
}




unsigned long getTimeStamp()
{
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  //delay(10);

  int cb = udp.parsePacket();
  if (!cb) {
    //Serial.println("no packet yet");
  }
  else {
    // Serial.println("packet received, length=");
    //Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Serial.print("Seconds since Jan 1 1900 = " );
    // Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    // Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears + 3600 * 5.5;
    // print Unix time:
    //Serial.println(epoch);
    // Serial.println();
    return epoch;
  }
}

unsigned long sendNTPpacket(IPAddress& address)
{
  //Serial.println("sending NTP packet...");
  //Serial.println();
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void callback(char* topic, byte* payload, unsigned int payloadLength) {
  Serial.print("callback invoked for topic: "); Serial.println(topic);

  if (strcmp (responseTopic, topic) == 0) {
    return; // just print of response for now
  }

  if (strcmp (rebootTopic, topic) == 0) {
    Serial.println("Rebooting...");
    ESP.restart();
  }

  if (strcmp (updateTopic, topic) == 0) {
    handleUpdate(payload);
  }
}




void readFlash(unsigned int sdid_sd)
{
  Serial.println("inside readFlash()");
  boolean flag_overwrite_flash = false;
  Spi_File sdid_flash_f = SPIFFS.open("/sdid.txt", "a+");
  if (sdid_flash_f) {
    Serial.println("file size : ");
    Serial.println(sdid_flash_f.size());
    Serial.print("Sd card id on flash is: ");
    String temp  = sdid_flash_f.readStringUntil('\n');
    sdid_flash = temp.toInt();
    Serial.println(sdid_flash);
    if (sdid_flash == sdid_sd)
    {
      Serial.println("SD card details match with previous record on flash");
    }
    if (sdid_flash == 0 || sdid_flash != sdid_sd)
    {
      Serial.println("Copying sd card no from sd card to esp");
      sdid_flash_f.close();
      flag_overwrite_flash = true;
    }
  }
  else {
    Serial.println("can not open sdid file from flash" );

  }

  if (flag_overwrite_flash)
  {
    SPIFFS.remove("/sdid.txt");
    Serial.println("old file removed");
    Serial.println("Creating new file on flash for storing sd no");
    Spi_File sdid_flash_f = SPIFFS.open("/sdid.txt", "w");
    if (sdid_flash_f)
    {
      sdid_flash_f.println(sdid_sd);

      sdid_flash_f.close();
    }
  }
  delay(50);
  Serial.println("leaving readFlash()");
}


void flash_failed_re_init()
{
  Serial.println("flash failed...reinitialising info file");
  Spi_File file = SPIFFS.open("/info.txt", "w");
  if (file)
  {
    Serial.println("inside info file");
    Serial.print("Start storing backup in this index: ");
    Serial.println("1");
    file.println("1");

    Serial.print("Start sending data from this file: ");
    Serial.println("1");
    file.println("1");

    Serial.print("no of files sent: ");
    Serial.println("0");
    file.println("0");

    file.close();
    Serial.println("new info file written to flash");
  }
  else
  {
    Serial.println("Flash failed.");
    ESP.restart();
  }

  Spi_File f9 = SPIFFS.open("/count.txt", "w");
  if (f9)
  {
    Serial.println("inside count file");
    Serial.print("Starting index of count: ");
    Serial.println("0");
    f9.println("0");
    f9.close();
  }
  else
  {
    Serial.println("Failed to open count file");
    ESP.restart();
  }
}


void getSerialData()
{
  count++;

  Spi_File f6 = SPIFFS.open("/count.txt", "r");
  if (f6)
  {
    Serial.println("Reading data from count file....");
    String s = f6.readStringUntil('\n');
    Serial.print("packet count read from flash is : ");
    Serial.println(s);
    packet_count = s.toInt();
    f6.close();
  }
  else
  {
    Serial.println("failed to open count file");
    Serial.println("ESP will restart");
    delay(500);
    ESP.reset();

  }

  if (Serial.available())
  {
    Serial.println("data on uart");
    payload = Serial.readString();
    //concat incoming string with timestamp
    Serial.println(payload);

    unsigned long TS = getTimeStamp();
    Serial.println("framing payload: ");
    payload1 = "{\"d\":{\"packet_count\":";
    payload1 += packet_count;
    //payload += ",\"V1\":";
    //payload += String(count);
    //payload += ",\"I1\":\"-1.0\",\"V2\":\"-1.0\",\"I2\":\"-1.0\",\"V3\":\"-1.0\",\"I3\":\"-1.0\",\"V4\":\"-1.0\",\"I4\":\"-1.0\",\"V5\":\"-1.0\",\"I5\":\"-1.0\",\"Phi1\":\"-1.0\",\"Phi2\":\"-1.0\",\"Phi3\":\"-1.0\",\"ExT\":\"-1.0\",\"T1\":\"-1.0\",\"TS\":";
    
    
    payload+= "\"TS\":";
    payload += String(TS);
    payload += "}}";
    payload1+=payload;
    payload =payload1;
    Serial.println(payload);
    packet_count++;

    Spi_File f7 = SPIFFS.open("/count.txt", "w");
    if (f7)
    {
      Serial.println("inside count file");
      Serial.print("Starting index of count: ");
      Serial.println("0");
      f7.println(String(packet_count));
      f7.close();
    }


  }
  else
  {
    Serial.println("Nothing on UART..will send error message");
    unsigned long TS = getTimeStamp();
    Serial.println("framing payload: ");
    payload2 = "{\"d\":{\"packet_count\":\"";
    payload2 += String(packet_count);
    
    //payload += "\",\"V1\":";
    //payload += String(random(200, 300));
    //payload += ",\"I1\":\"1.0\",\"V2\":\"-1.0\",\"I2\":\"-1.0\",\"V3\":\"-1.0\",\"I3\":\"-1.0\",\"V4\":\"-1.0\",\"I4\":\"-1.0\",\"V5\":\"-1.0\",\"I5\":\"-1.0\",\"Phi1\":\"-1.0\",\"Phi2\":\"-1.0\",\"Phi3\":\"-1.0\",\"ExT\":\"-1.0\",\"T1\":\"-1.0\",\"TS\":";
    //payload += String(TS);
    //payload += "}}";
    /*
    payload += ",\"I1\":";
    payload += String(random(1, 10));
    payload += ",\"V2\":";
    payload += String(random(200, 300));
    payload += ",\"I2\":";
    payload += String(random(1, 10));
    payload += ",\"V3\":";
    payload += String(random(200, 300));
    payload += ",\"I3\":";
    payload += String(random(1, 10));
    payload += ",\"V4\":";
    payload += String(random(200, 300));
    payload+=",\"Phi3\":";
    payload+= String(random(30,60));
    payload+= ",\"ExT\":\"50.0\",\"T1\":\"37\",\"TS\":";
    */
    payload+="\"TS\":";
    payload+=String(TS);
    payload+="}}";
    payload2 += payload;
    payload=payload2; 
    Serial.println(payload);
    packet_count++;
    
    fs::Spi_File f8 = SPIFFS.open("/count.txt", "w");
    if (f8)
    {
      Serial.println("inside count file");
      f8.println(String(packet_count));
      f8.close();
      Serial.println("updated count");
    }
  }
  Serial.println("Sending payload: ");
  //Serial.println(payload);
}


void handleUpdate(byte* payload) {
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((char*)payload);
  if (!root.success()) {
    Serial.println("handleUpdate: payload parse FAILED");
    return;
  }
  Serial.println("handleUpdate payload:"); root.prettyPrintTo(Serial); Serial.println();

  JsonObject& d = root["d"];
  JsonArray& fields = d["fields"];
  for (JsonArray::iterator it = fields.begin(); it != fields.end(); ++it) {
    JsonObject& field = *it;
    const char* fieldName = field["field"];
    if (strcmp (fieldName, "metadata") == 0) {
      JsonObject& fieldValue = field["value"];
      if (fieldValue.containsKey("publishInterval")) {
        publishInterval = fieldValue["publishInterval"];
        Serial.print("publishInterval:"); Serial.println(publishInterval);
      }
    }
  }
}

void readSD()
{
  delay(10);
  Serial.println("Inside readSD()");
  String temp = "";
  char data = 'x';
  Serial.println("Before sdid read ");
  File sdid_sd_f = SD.open("sdid.txt");
  if (sdid_sd_f)
  {
    while (data != '\n')
    {
      data = sdid_sd_f.read();
      temp += data;
    }
    sdid_sd = temp.toInt();
    Serial.print("sdid read from sd card is : ");
    Serial.println(sdid_sd);
    delay(50);
    sdid_sd_f.close();
    Serial.println("leaving readSD()");
    delay(10);
  }
  else
  {
    Serial.println("Failed to open sdid file.");
  }
}


void initManagedDevice() {
  if (client.subscribe("iotdm-1/response")) {
    Serial.println("subscribe to responses OK");
  } else {
    Serial.println("subscribe to responses FAILED");
  }

  if (client.subscribe(rebootTopic)) {
    Serial.println("subscribe to reboot OK");
  } else {
    Serial.println("subscribe to reboot FAILED");
  }

  if (client.subscribe("iotdm-1/device/update")) {
    Serial.println("subscribe to update OK");
  } else {
    Serial.println("subscribe to update FAILED");
  }

  if (client.subscribe(factoryResetTopic)) {
    Serial.println("subscribe to factory reset OK");
  } else {
    Serial.println("subscribe to factory reset FAILED");
  }



  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& metadata = d.createNestedObject("metadata");
  metadata["publishInterval"] = publishInterval;
  JsonObject& supports = d.createNestedObject("supports");
  supports["deviceActions"] = true;

  char buff[500];
  root.printTo(buff, sizeof(buff));
  Serial.println("publishing device metadata:");
  Serial.println(buff);
  if (client.publish(manageTopic, buff)) {
    Serial.println("device Publish ok");
  } else {
    Serial.print("device Publish failed:");
  }
}

