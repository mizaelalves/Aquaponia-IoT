
#include <Arduino.h>
#if defined(ESP32)
#endif
#include <Firebase_ESP_Client.h>
#include "DHT.h"

#include <NTPClient.h>
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
#define USER_EMAIL "mizaelbna@hotmail.com"
#define USER_PASSWORD "projetoaquaponia" 
// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = true;
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
int hora;
int minutos;
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

String currentTime;
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
void initRele(int value,String name)
{
  if (value == 1)
  {
    digitalWrite(rele1, HIGH);
    Serial.println(name+" ligado");
  }
  else
  {
    digitalWrite(rele1, LOW);
    Serial.println(name+" desligado");
  }
}
void initRele2(int value, String name)
{
  if (value == 1)
  {
    digitalWrite(rele2, HIGH);
    Serial.println(name+" ligado");
  }
  else
  {
    digitalWrite(rele2, LOW);
    Serial.println(name+" desligado");
  }
}

void initFirebase()
{

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  String base_path = "/UsersData/";
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  /* Sign up */
  /*
  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
    Serial.println("Error");
  }
 */
  //Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
}

// Função set do firebase
void setFunction(String sensorName, int sensorValue)
{
  if (Firebase.RTDB.setInt(&fbdo, "/" + sensorName, sensorValue))
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

void initRoutine(int value)
{
  //Serial.println(ntp.getHours());
  //Serial.println(ntp.getMinutes());
  if (value == 1)
  {
    //String temperature = String(dht.readTemperature());
    //String humidity = String(dht.readHumidity());
    setFunction("atuadores/motor/motor1", 1);
    setFunction("atuadores/solenoide/solenoide1", 1);
    Serial.println("***Atenção***ROTINA INICIADA***");
  }
  else{
    Serial.println("Esperando rotina!!!");
  }
}

/*
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
*/

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
  // solenoide

  initRele(getFunction("/atuadores/solenoide/solenoide1", "solenoide"), "solenoide");
  // motor
  initRele2(getFunction("/atuadores/motor/motor1", "motor"), "motor");
  //setFunction("atuadores/motor/motor1", "1");
  //initRoutine(getFunction("/rotina", "rotina"));
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
  // motor(getFunction("/motores/motor/1/power", "motor 1"));
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

int convertInSeconds(int interval){
  int hourToSecond = ntp.getHours() * 60 * 60;
  int minutesToSecond = ntp.getMinutes() * 60;
  int total = hourToSecond + minutesToSecond;
  //Serial.println(hourToSecond);
  return total;
}

void setup()
{
  wifiMulti.addAP("Redmi", "fenix2332");
  wifiMulti.addAP("FabLab", "fablabmaker");
  wifiMulti.addAP("BETEL", "universitario");
  wifiMulti.addAP("CASA", "23lafenix");
  wifiMulti.addAP("LABHARD", "ufpaLabHard");
  Serial.begin(9600);
  initWifi();
  
  initFirebase();
  dht.begin();
  ntp.begin();
  ntp.setTimeOffset(-10800);
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

long previousMillis = 0;
void loop()
{
  ntp.update();
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();

    // setDataFirebase();
    Serial.println("--Projeto Aquaponia *Firebase*--");
    getDataFirebase();
    //if(getFunction("/rotina/estado", "solenoide") == 1){
     // initRoutine(1);
     sleep(20000);
   // }
    //currentTime = ntp.getHours()+":"+ntp.getMinutes()+":"+ntp.getSeconds();
    
    convertInSeconds();
  }
}

