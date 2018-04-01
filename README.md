# IoT-with-ESP8268: Used to send data to bluemix server.SD card is used as storage device in offline mode.

Just an example and to some extent, a readable code. Not optimized, should not to be directly used in a product.
Combines IoT Starter code by Ant Elder, Some Example codes for getting,NTP Timestamps,Storage in SD card
License: Apache License v2
Used ESP8266 to send data collected from Solar Inverter processor to  Bluemix NoSQL Server

Following cases were tested in final code.Data was continuously sent over UART after interval of 10 seconds

A) Normal Operation:

Inits:- Void setup() of the code. Initializes MQTT, Wi-Fi in both AP and client mode, SD CARD,UART,Websocket Request parameters.
  Finally, connects to Wi-Fi.then  connects to the said DB.
Loop Section:This portion contains void loop() and functions called in it.
1. Wait for serial data to arrive.
2. When data arrives, parse it and create a json string
3. Add timestamp and Packet Count to the json string(Packet count is stored in and retrived from SPI FLASH so that even after reboot, packet count remains unperturbed)
4. Try to send json string to server.
5. If sent successfully, check SD card.
6. If not, store it in sd card and wait for UART data.(Reconnection attempts take place in the background if connection is lost)
7. If previous string sent sucessfully and If there is a string in SD card try sending that string to the server.
8. If string sent successfully, delete the string from SD card and wait for serial data to arrive
9. If not,wait for Serial Data to arrive. 


B) Communication with some other device(Preferably Android): When SSID or Password of Access point changes(Maybe because when user changes network provider or router)
This was included in the void loop section, but implementation with a software interrupt sounds more intuitive.
 
1. At any point in the void loop(preferably in the beginning) check for websocket connection request for AP mode. 
   By adding some more flags in the library, this step was detected as early as three way handshaking was completed
2. If no device is connected, Normal Operation 
3. If a device is connected, then receive new credentials(SSID,PASSWORD) from device over websocket and store the credentials in EEPROM/SPI-FLASH
4. Reboot and connect using new credentials
    
