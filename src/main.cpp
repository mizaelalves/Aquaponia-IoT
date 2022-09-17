
#include <Arduino.h>
#if defined(ESP32)
#endif
#include <Firebase_ESP_Client.h>
#include "DHT.h"
//#include <WiFi.h>
//#include <WiFiMulti.h>
//#include <initWiFi.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define FIREBASE_USE_PSRAM
#include <WiFiMulti.h>
#include <WiFi.h>

// Insert Firebase project API Key
#define API_KEY "AIzaSyBtaKEQdxWfDPhFi1DSlDkWWfpGFjDSqDE"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://aquaponia-iot-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;
// dht
DHT dht(23, DHT22);
WiFiMulti wifiMulti;

// Init wifiInit();

int ledValue = 32;
int motor1pin1 = 26;
int motor1pin2 = 25;
int enable1Pin = 5;
unsigned long sendDataPrevMillis = 0;
int count1 = 0;
int count2 = 0;
int intValue;
String stringValue;
float floatValue;


float temperatura;
float umidade;
int rele1 = 22;
int rele2 = 19;
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

void initWifi()
{
  /*
   *Serial.print("Connecting to Wi-Fi");
   *while (WiFi.status() != WL_CONNECTED)
   *{
   *  Serial.print(".");
   *  delay(300);
   *}
   **/
  Serial.println("Esperando conexão...");
  if (wifiMulti.run() == WL_CONNECTED)
  {
    Serial.println(WiFi.SSID());
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Função de inicialização do rele
void initRele(int value)
{
  if (value == 1)
  {
    digitalWrite(rele1, HIGH);
    Serial.println("Rele ligado");
  }
  else
  {
    digitalWrite(rele1, LOW);
    Serial.println("Rele desligado");
  }
}
void initRele2(int value)
{
  if (value == 1)
  {
    digitalWrite(rele2, HIGH);
    Serial.println("Rele ligado");
  }
  else
  {
    digitalWrite(rele2, LOW);
    Serial.println("Rele desligado");
  }
}

void initFirebase()
{

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
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

// Função set do firebase
void setFunction(String sensorName, String sensorValue)
{
  if (Firebase.RTDB.setString(&fbdo, "/" + sensorName, sensorValue))
  {
    sensorName = sensorValue;
    Serial.println(sensorName);
    Serial.println("Informação enviada");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());

    Serial.println(sensorName + ": " + sensorValue);
  }
}

int getFunction(String path, String name)
{
  if (Firebase.RTDB.getInt(&fbdo, "/" + path))
  {
    if (fbdo.dataType() == "int")
    {
      Serial.println("Dados recebidos do " + name);
      int value = fbdo.intData();
      return value;
    }
    else
    {
      Serial.println(name + " ERROR " + fbdo.errorReason());
    }
  };
  return 0;
}

void setDataFirebase()
{
  if (dht.readTemperature() > 0 && dht.readHumidity() > 0)
  {
    String temperature = String(dht.readTemperature());
    String humidity = String(dht.readHumidity());
    setFunction("temperatura", temperature);
    setFunction("umidade", humidity);
  }
  else
  {
    stringValue = "Esperando informações do DHT...";
    Serial.println(stringValue);
  }
}

void motor(int power)
{
  // Move the DC motor forward at maximum speed
  if (power == 1)
  {

    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, HIGH);
  }
  else
  {
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor1pin2, LOW);
  }
}
void led(int ledV)
{
  // Move the DC motor forward at maximum speed
  if (ledV == 1)
  {

    digitalWrite(ledValue, LOW);
  }
  else
  {
    digitalWrite(ledValue, LOW);
  }
}

void getDataFirebase()
{
  //solenoide
  //initRele(getFunction("/atuadores/solenoide/solenoide1", "solenoide"));
  //motor
  //initRele2(getFunction("/atuadores/motor/motor1", "motor"));
  //setFunction("atuadores/motor/motor1", "1");

  
  /*
  if (Firebase.RTDB.getInt(&fbdo, "/motores/solenoide/1"))
  {
    if (fbdo.dataType() == "int")
    {
      intValue = fbdo.intData();
      initRele(intValue);
    }
    else
    {
      Serial.println("TESTE" + fbdo.errorReason());
    }
  }
  */
  // led(getFunction("/teste/led/", "ledValue"));
  //motor(getFunction("/motores/motor/1/power", "motor 1"));
  /*
  if (Firebase.RTDB.getString(&fbdo, "/motores/motor1/power"))
  {
    if (fbdo.dataType() == "string")
    {
      stringValue = fbdo.stringData();
      motor(stringValue);
      Serial.println("motor " + stringValue);
    }
    else
    {
      Serial.println("TESTE" + fbdo.errorReason());
    }
  }
  */
}

void setup()
{
  wifiMulti.addAP("Redmi", "fenix2332");
  wifiMulti.addAP("FabLab", "fablabmaker");
  wifiMulti.addAP("BETEL", "universitario");
  wifiMulti.addAP("CASA", "23lafenix");
  Serial.begin(9600);
  initWifi();
  initFirebase();
  dht.begin();
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(ledValue, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  // configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(enable1Pin, pwmChannel);

  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  pinMode(2, OUTPUT);
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
