#include <Arduino.h>
#include <WiFiMulti.h>
#include <WiFi.h>
#include <InitWiFi.h>
WiFiMulti wifiMulti;

Init::Init(){

}

void Init::wifiInit()
{
  
  /*
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  */
  Serial.println("Esperando conexão...");
  if (wifiMulti.run() == WL_CONNECTED)
  {
    Serial.println(WiFi.SSID());
  }
  Serial.println("--------------------");
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("--------------------");
}