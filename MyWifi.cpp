#include "WiFi.h"
#include "MyWifi.h"

const short timeoutLimit = 20; //measured in number of 200 ms increments

boolean myWiFiOpenNew(){
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    /*
     * Possible returns from WiFi.encryptionType()
     * WIFI_AUTH_WEP             1
     * WIFI_AUTH_WPA_PSK         2
     * WIFI_AUTH_WPA2_PSK        3   NETGEAR44
     * WIFI_AUTH_WPA_WPA2_PSK    4   FARMERJ
     * WIFI_AUTH_WPA2_ENTERPRISE 5
     * WIFI_AUTH_MAX             6
    //Find wifi networks
    int n = WiFi.scanNetworks(); //number of networks found
    for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            snprintf(buff1,BUFF1SIZE,"%d",WiFi.encryptionType(i));
            Serial.println(buff1);
            delay(10);
     }
     */
     
    /*
     * Possible Returns from WiFi.status():
     * WL_CONNECTED: 3
     * WL_NO_SHIELD: 255
     * WL_IDLE_STATUS: 0
     * WL_NO_SSID_AVAIL: 1
     * WL_SCAN_COMPLETED: 2
     * WL_CONNECT_FAILED: 4
     * WL_CONNECTION_LOST: 5
     * WL_DISCONNECTED: 6
     */
    WiFi.begin("put name of wifi network here", "put password to wifi network here");

    
    for(short k = 0; k < timeoutLimit; k++){
        int status = WiFi.status();
        if(status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_NO_SSID_AVAIL || status == WL_SCAN_COMPLETED){
            //Serial.println("Waiting");
            delay(200);
        }
        else if(status == WL_CONNECTED){
            //Serial.println("Connected");
            return true;
        }
        else if(status == WL_NO_SHIELD || status == WL_CONNECT_FAILED || status == WL_CONNECTION_LOST){
            Serial.println("An error occurred");
            return false;
        }
        else{
            Serial.println("Unknown Wifi status");
            return false;
        }
    }
    Serial.println("Wifi connect timed out");
    return false;
}

boolean myWiFiConnect(){
  if(WiFi.status() == WL_CONNECTED)
    return true;
  else
    return myWiFiOpenNew();
}

void myWiFiDisconnect(){
  WiFi.disconnect();
}
