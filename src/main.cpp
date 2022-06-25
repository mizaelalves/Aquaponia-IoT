
#include <Arduino.h>
#if defined(ESP32)
#endif
#include <Firebase_ESP_Client.h>
#include "DHT.h"
#include <WiFi.h>
#include <WiFiMulti.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
#define FIREBASE_USE_PSRAM

// Insert Firebase project API Key
#define API_KEY "AIzaSyBtaKEQdxWfDPhFi1DSlDkWWfpGFjDSqDE"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://aquaponia-iot-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// dht
DHT dht(23, DHT22);
WiFiMulti wifiMulti;



unsigned long sendDataPrevMillis = 0;
int count1 = 0;
int count2 = 0;
int intValue;
String stringValue;
float floatValue;
bool signupOK = false;
int solenoide = 23;
float temperatura;
float umidade;

void initWifi()
{
  /*
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  */
 if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println(WiFi.SSID());
    }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
void initFirebase()
{

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
}
void setDataFirebase()
{/*
    float temperatura;

    float umidade;
  */
  // Write an Int number on the database path test/int
  // dht
  if (Firebase.RTDB.setString(&fbdo, "sensores/dht/temperatura", temperatura) && Firebase.RTDB.setString(&fbdo, "sensores/dht/humidade", umidade))
  {
    
    if (dht.readTemperature() > 0 && dht.readHumidity() > 0)
    {
      temperatura = dht.readTemperature();
      umidade = dht.readHumidity();
      Serial.println(temperatura);
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      stringValue = "Esperando informações do DHT...";
      Serial.println(stringValue);

    }
  }
  else
  {
    Serial.println("FAILED DHT");
    Serial.println("REASON: " + fbdo.errorReason());
  }

  /*
    if (Firebase.RTDB.setFloat(&fbdo, "motores/motor/1", temperatura)) {

    }
    else {
      Serial.println("FAILED DHT");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    */
}

void getDataFirebase()
{
  if (Firebase.RTDB.getInt(&fbdo, "/teste/led/ledValue"))
  {
    if (fbdo.dataType() == "int")
    {
      intValue = fbdo.intData();

      if (intValue == 1)
      {
        Serial.println(intValue);
        digitalWrite(2, HIGH);
      }
      else
      {
        digitalWrite(2, LOW);
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
  if (Firebase.RTDB.getInt(&fbdo, "/motores/solenoide/1"))
  {
    if (fbdo.dataType() == "int")
    {
      intValue = fbdo.intData();
      Serial.println(intValue);
    }
    else
    {
      Serial.println("TESTE" + fbdo.errorReason());
    }
  }
}

void setup()
{
  wifiMulti.addAP("Redmi", "fenix2332");
  wifiMulti.addAP("FabLab Camtuc", "11223344");
  wifiMulti.addAP("BETEL", "universitario");
  wifiMulti.addAP("CASA", "23lafenix");

  //WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  pinMode(solenoide, OUTPUT);
  pinMode(2, OUTPUT);
  Serial.begin(9600);
  dht.begin();
  initWifi();
  initFirebase();
}

void loop()
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    setDataFirebase();
    getDataFirebase();
  }
}
