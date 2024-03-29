#include <Arduino.h>
#if defined(ESP32)
#endif
#include <Firebase_ESP_Client.h>
#include "SECRET.h"
#include <NTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
unsigned long ultima_atualizacao = 0;             // Armazena o tempo da última atualização
const unsigned long intervalo_atualizacao = 3000; // Intervalo de atualização em milissegundos (5 segundos)
static bool estadoRegaPlantas = false;

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define FIREBASE_USE_PSRAM
#include <WiFiMulti.h>
#include <WiFi.h>

//#include <Servo.h> // Include the servo library
//Servo myservo;     // create servo object to control our servo

// ----------------- ATUADORES E SENSORES ------------------
// Componentes do Aquario
//int motorServo = 34;
//  sensores
int nivelBaixoAquario = 4; // Sensor nivel baixo
int nivelAltoAquario = 2;   // Sensor nivel alto
//  atuadores
int valvulaAquario = 27; // valvula que enche o aquario
int bombaAquario = 13;   // bomba que envia agua para a tubulacao

// Variaveis de estado Aquario
bool estadoBombaAquario = false;
bool estadoValvulaAquario = false;
bool estadoSNAAquario = false;
bool estadoSNBAquario = false;

// pH variaveis
const int analogInPin = 36; 
int sensorValor = 0; 
unsigned long int avgValue; 
float b;
int buf[10],temp;

// Componentes da Tubulacao
int valvulaDescarte = 25;   // Descarta agua suja
int valvulaHidroponia = 26; // Permite que a agua va para as plantas
// Variaveis de estado Tubulacao
bool estadoValvulaDescarte = false;
bool estadoValvulaHidroponia = false;

bool rotina_iniciada = false;

// Componentes da Cisterna
int nivelBaixoCisterna = 19; // Sensor nivel baixo
int nivelAltoCisterna = 18;  // Sensor nivel alto
int bombaCisterna = 14;      // Manda agua para o decompositor
// Variaveis de estado bombaBox
bool estadoBombaCisterna = false;
bool estadoSensorNivelBaixoCisterna = false;
bool estadoSensorNivelAltoCisterna = false;

// variaveis para o metodo regarPlantas()
// variaveis para o metodo reabastecerAquario()
// const long tempoRegaLigado = 180000;    // Intervalo em milissegundos para ligar o LED (3min)
// const long tempoRegaDesligado = 900000; // Intervalo em milissegundos para desligar o LED (15min)
//const int tempoMinInicioRega = 3;
//const int tempoMinDesligaRega = 15;

const long tempoRegaLigado = 30000;            //(tempoMinInicioRega * 60) * 1000;   // Intervalo em milissegundos para ligar o LED (10s)
const long tempoRegaDesligado = 30000;        //(tempoMinDesligaRega * 60) * 1000; // Intervalo em milissegundos para desligar o LED (10s)
unsigned long anterirTarefaReabastecimento = 0; // Variável para armazenar o tempo anterior
// bool valveOn = false;                           // Estado da válvula (Ligado/Desligado)

bool statusEnvioRega = false;

// ------------- FIM DOS ATUADORES E SENSORES -------------------
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = true;
WiFiMulti wifiMulti;

// Init wifiInit();
unsigned long sendDataPrevMillis = 0;
WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000); /* Cria um objeto "NTP" com as configurações.utilizada no Brasil */
String hora;

String horaInicioRotina;
String horaFimRotina;

String currentTime;

unsigned long enviaDadosPHMillisAnterior = 0;

String base_path = "/data_esp1/";
// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperaturaAquario";
String phPath = "/phAquario";

String timePath = "/timestamp";
// Parent Node (to be updated in every loop)
String parentPath;
int timestamp;
// Timer variables (send new readings every three minutes)
unsigned long sendDataFirebasePrevMillis = 0;
unsigned long timerDelay = 180000;

// Inicia conexao wifi
void initWifi()
{
  Serial.print("Conectando ao WiFi...");
  lcd.setCursor(0, 0);
  lcd.print("Conectando ao WiFi:");
  WiFi.begin(SSID, KEY);
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED and tentativas <= 30)
  {

    delay(1000); // aguarda 500ms antes de tentar novamente
    lcd.setCursor(0, 1);
    lcd.print("Aguarde...");
    Serial.print(".");
    tentativas += 1;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conexao OK!");
    lcd.setCursor(0, 1);
    lcd.print("Endereço IP: ");
    lcd.setCursor(0, 2);
    lcd.print(WiFi.localIP());
    Serial.println("Conexão estabelecida!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    delay(2000);
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha na conexao.");
    Serial.println("Falha na conexão. Timeout atingido.");
    delay(2000);
  }
}

void initFirebase()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando Firebase");
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */

  config.timeout.wifiReconnect = 10 * 1000;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(false);
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = String(base_path + uid + "/readings");

  if (Firebase.ready())
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase ON :)");
    lcd.setCursor(0, 1);
    lcd.print("uid: " + uid);
  }

  delay(1000);
}


void reabastecerAquario()
{
  // 1 = nao detectou
  // 0 = detectou
  // Em LOW o LED do relé fica aceso
  Serial.println();
  Serial.print("nivel baixo aquario=");
  Serial.println(digitalRead(nivelBaixoAquario));
  Serial.print("nivel alto aquario=");
  Serial.println(digitalRead(nivelAltoAquario));
  if (digitalRead(nivelBaixoAquario) == 1 and digitalRead(nivelAltoAquario) == 1)
  {                                     // Se nao detectar agua
    Serial.println("reabastecendo aquario");
    digitalWrite(valvulaAquario, HIGH); // liga valvula para reabastecer aquario
    estadoValvulaAquario = true;
    estadoSNAAquario = false;
    estadoSNBAquario = false;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Define valvula aquario... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/valvulaAquario"), estadoValvulaAquario) ? "valvula aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel baixo... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelBaixoAquario"), estadoSNBAquario) ? "sensor baixo aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel alto... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelAltoAquario"), estadoSNAAquario) ? "sensor alto aquario ok" : fbdo.errorReason().c_str());
    }
  }
  else if (digitalRead(nivelBaixoAquario) == 0 and digitalRead(nivelAltoAquario) == 0)
  { // se detectar agua
    
    digitalWrite(valvulaAquario, LOW);
    Serial.println("Valvula do aquario desligada");
    estadoValvulaAquario = false;
    estadoSNAAquario = true;
    estadoSNBAquario = true;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Define valvula aquario... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/valvulaAquario"), estadoValvulaAquario) ? "valvula aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel baixo... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelBaixoAquario"), estadoSNBAquario) ? "sensor baixo aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel alto... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelAltoAquario"), estadoSNAAquario) ? "sensor alto aquario ok" : fbdo.errorReason().c_str());
    }
  }
  else if (digitalRead(nivelBaixoAquario) == 0 and digitalRead(nivelAltoAquario) == 1)
  {
    estadoSNAAquario = false;
    estadoSNBAquario = true;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Define valvula aquario... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/valvulaAquario"), estadoValvulaAquario) ? "valvula aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel baixo... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelBaixoAquario"), estadoSNBAquario) ? "sensor baixo aquario ok" : fbdo.errorReason().c_str());
      Serial.printf("Define sensor nivel alto... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/sensorNivelAltoAquario"), estadoSNAAquario) ? "sensor alto aquario ok" : fbdo.errorReason().c_str());
    }
  }
}

// OBS: sensor nivel baixo invertido
void encherDecantador()
{
  Serial.println();
  Serial.print("nivel baixo p/ cisterna=");
  Serial.println(digitalRead(nivelBaixoCisterna));
  Serial.print("nivel alto p/ cisterna=");
  Serial.println(digitalRead(nivelAltoCisterna));
  if (digitalRead(nivelAltoCisterna) == 1 and digitalRead(nivelBaixoCisterna) == 1 and !estadoBombaCisterna)
  {                                    // Se sensor nivel alto detectar agua
    digitalWrite(bombaCisterna, HIGH); // liga a bomba
    Serial.println("Bomba Cisterna ligada");
    estadoBombaCisterna = true;
    estadoSensorNivelAltoCisterna = true;
    estadoSensorNivelBaixoCisterna = true;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Definindo nivel alto cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelAltoCisterna"), estadoSensorNivelAltoCisterna) ? "sensor alto cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo nivel baixo cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelBaixoCisterna"), estadoSensorNivelBaixoCisterna) ? "sensor baixo cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo bomba cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/bombaCisterna"), estadoBombaCisterna) ? "bomba cisterna ok" : fbdo.errorReason().c_str());
    }
  }
  else if (digitalRead(nivelAltoCisterna) == 0 and digitalRead(nivelBaixoCisterna) == 1 and estadoBombaCisterna)
  { // se o nivel da agua estiver abaixando e estiver no meio dos sensores 
    estadoSensorNivelAltoCisterna = false;
    estadoSensorNivelBaixoCisterna = true;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Definindo nivel alto cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelAltoCisterna"), estadoSensorNivelAltoCisterna) ? "sensor alto cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo nivel baixo cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelBaixoCisterna"), estadoSensorNivelBaixoCisterna) ? "sensor baixo cisterna ok" : fbdo.errorReason().c_str());
    }
  }
  else if (digitalRead(nivelAltoCisterna) == 0 and digitalRead(nivelBaixoCisterna) == 0 and estadoBombaCisterna)
  { // Se os sensores nao tiverem agua e a bomba estava ligada
    digitalWrite(bombaCisterna, LOW);
    Serial.println("Bomba Cisterna desligada");
    estadoBombaCisterna = false;
    estadoSensorNivelAltoCisterna = false;
    estadoSensorNivelBaixoCisterna = false;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      Serial.printf("Definindo bomba cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/bombaCisterna"), estadoBombaCisterna) ? "bomba cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo nivel alto cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelAltoCisterna"), estadoSensorNivelAltoCisterna) ? "sensor alto cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo nivel baixo cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelBaixoCisterna"), estadoSensorNivelBaixoCisterna) ? "sensor baixo cisterna ok" : fbdo.errorReason().c_str());
    }
  }
  else{
    Serial.println("Bomba Cisterna desligada");
    estadoBombaCisterna=false;
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK){
      Serial.printf("Definindo nivel alto cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelAltoCisterna"), estadoSensorNivelAltoCisterna) ? "sensor alto cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo nivel baixo cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/sensorNivelBaixoCisterna"), estadoSensorNivelBaixoCisterna) ? "sensor baixo cisterna ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo bomba cisterna... %s\n", Firebase.RTDB.setBool(&fbdo, F("Cisterna/bombaCisterna"), estadoBombaCisterna) ? "bomba cisterna ok" : fbdo.errorReason().c_str());
    }
  }
}

void regarPlantas()
{
  static unsigned long startMillis = 0; // Obtém o tempo atual em milissegundos
  if (estadoRegaPlantas == false && millis() - startMillis >= tempoRegaDesligado || estadoRegaPlantas == false && startMillis == 0)
  {
    estadoRegaPlantas = true;
    startMillis = millis();

    Serial.println();
    Serial.println("*** Iniciando Rotina *****");
    rotina_iniciada = true;
    digitalWrite(valvulaDescarte, LOW);
    digitalWrite(valvulaHidroponia, HIGH);
    digitalWrite(bombaAquario, LOW);//bomba aquario se ativa em LOW
    Serial.println();
    Serial.println("Valvula Hidroponia ligada");
    Serial.println("Valvula descarte desligada");
    Serial.println("Bomba aquario ligada");
    estadoValvulaDescarte = false;
    estadoValvulaHidroponia = true;
    estadoBombaAquario = true;
    
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      hora = ntp.getFormattedTime();
      horaInicioRotina = hora;

      Serial.printf("Iniciando rotina... %s\n", Firebase.RTDB.setBool(&fbdo, F("RotinaRegaPlantas/valor"), true) ? "rotina iniciada" : fbdo.errorReason().c_str());
      Serial.printf("Definindo horario ultima rotina... %s\n", Firebase.RTDB.setString(&fbdo, F("RotinaRegaPlantas/ultimaRotinaHora"), horaInicioRotina) ? "horario enviado" : fbdo.errorReason().c_str());

      Serial.printf("Definindo estado da valvula de descarte... %s\n", Firebase.RTDB.setBool(&fbdo, F("Tubulacao/valvulaDescarte"), estadoValvulaDescarte) ? "valvula descarte ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo estado da valvula de hidroponia... %s\n", Firebase.RTDB.setBool(&fbdo, F("Tubulacao/valvulaHidroponia"), estadoValvulaHidroponia) ? "valvula hidroponia ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo estado da bomba do aquario...... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/bombaAquario"), estadoBombaAquario) ? "bomba aquario ok" : fbdo.errorReason().c_str());
    }
  }
  else if (estadoRegaPlantas == true && millis() - startMillis >= tempoRegaLigado)
  {
    estadoRegaPlantas = false;
    startMillis = millis();
    digitalWrite(bombaAquario, HIGH);//bomba aquario desativa em HIGH
    digitalWrite(valvulaHidroponia, LOW);
    estadoValvulaHidroponia = false;
    estadoBombaAquario = false;
    // parou de regar o led do relé acende
    Serial.println("Bomba aquario desligada");
    Serial.println("Valvula Hidroponia desligada");
    Serial.println("*** Desligando rotina *****");
    Serial.println();
    if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
    {
      hora = ntp.getFormattedTime();
      horaFimRotina = hora;
      Serial.printf("Definindo horario ultima rotina... %s\n", Firebase.RTDB.setString(&fbdo, F("RotinaRegaPlantas/fimRotinaHora"), horaFimRotina) ? "horario enviado" : fbdo.errorReason().c_str());

      Serial.printf("Deslingando rotina... %s\n", Firebase.RTDB.setBool(&fbdo, F("RotinaRegaPlantas/valor"), false) ? "rotina desligada" : fbdo.errorReason().c_str());
      Serial.printf("Definindo estado da valvula de hidroponia... %s\n", Firebase.RTDB.setBool(&fbdo, F("Tubulacao/valvulaHidroponia"), false) ? "valvula hidroponia ok" : fbdo.errorReason().c_str());
      Serial.printf("Definindo estado da bomba do aquario...... %s\n", Firebase.RTDB.setBool(&fbdo, F("Aquario/bombaAquario"), false) ? "bomba aquario ok" : fbdo.errorReason().c_str());
    }
  }
}

void verificaPH()
{
  for(int i=0;i<10;i++) 
  { 
    buf[i]=analogRead(analogInPin);
    //Serial.print("AD = ");
    //Serial.println(buf[i]);
    delay(10);
  }
  for(int i=0;i<9;i++)
    {
    for(int j=i+1;j<10;j++)
      {
      if(buf[i]>buf[j])
        {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)
    avgValue+=buf[i];
  //float pHVol=(float)avgValue*5.0/1024/6;
  float pHVol=(float)avgValue/6*3.4469/4095;
  //Serial.print("v = ");
  //Serial.println(pHVol);

  float phValor = -5.70 * pHVol + 21.34;    
  //float phValue = 7 + ((2.5 - pHVol) / 0.18);
  Serial.println();
  Serial.print("Ph=");
  Serial.println(phValor);
  
  if (WiFi.status() == WL_CONNECTED and Firebase.ready() && signupOK)
  {
    Serial.printf("Definindo ph... %s\n", Firebase.RTDB.setFloat(&fbdo, F("Aquario/ph"), phValor) ? "ph enviado" : fbdo.errorReason().c_str());
  }
}

unsigned long startTime = 0;
//bool alimentacao7hOn = false;
//bool alimentacao17hOn = false;

//const unsigned long rotationTime = 5000; // Tempo de rotação em milissegundos
//bool isServoOn = false;
/*
void alimentacao()
{
  ntp.update();
  int currentHour = ntp.getHours();
  int currentMinute = ntp.getMinutes();

  if (currentHour == 20 && currentMinute == 35 && !alimentacao7hOn)
  {
    Serial.println(currentHour);
    Serial.println(currentMinute);
    alimentacao7hOn = true;
    alimentacao17hOn = false;

    if (!isServoOn)
    {
      startTime = millis();
      myservo.write(180); // Posição para girar o servo (exemplo: 90 graus)
      Serial.println("Alimentou");
      Serial.printf("Alimentou... %s\n", Firebase.RTDB.setBool(&fbdo, F("/aliementacao/alimentacao09"), true) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Alimentou... %s\n", Firebase.RTDB.setBool(&fbdo, F("/aliementacao/alimentacao17"), false) ? "ok" : fbdo.errorReason().c_str());

      isServoOn = true;
    }
  }

  if (currentHour == 20 && currentMinute == 36 && !alimentacao17hOn)
  {
    alimentacao7hOn = false;
    alimentacao17hOn = true;

    if (!isServoOn)
    {
      startTime = millis();
      myservo.write(180); // Posição para girar o servo (exemplo: 90 graus)
      Serial.println("Alimentou");
      Serial.printf("Alimentou... %s\n", Firebase.RTDB.setBool(&fbdo, F("/aliementacao/alimentacao07"), false) ? "ok" : fbdo.errorReason().c_str());
      Serial.printf("Alimentou... %s\n", Firebase.RTDB.setBool(&fbdo, F("/aliementacao/alimentacao17"), true) ? "ok" : fbdo.errorReason().c_str());
      isServoOn = true;
    }
  }

  if (millis() - startTime >= rotationTime && isServoOn)
  {
    myservo.write(0); // Desligar o servo (posição 0 graus)
    Serial.println("Desligou");
    isServoOn = false;
  }
}
*/
void mostrarLCD(int sensorNivelBaixoCisterna, int sensorNivelAltoCisterna, int sensorNivelBaixoAquario, int sensorNivelAltoAquario)
{
  // Verificar se já passaram 6 segundos desde a última atualização
  if (millis() - ultima_atualizacao >= intervalo_atualizacao || ultima_atualizacao == 0)
  {
    ultima_atualizacao = millis(); // Atualizar o tempo da última atualização

    // Exibir as informações de pH e temperatura
    // lcd.setCursor(0, 0);
    // lcd.printf("pH: %.f", ph);
    // lcd.setCursor(0, 1);
    // lcd.printf("Temp: %.f", temperatura);

    // Verificar se a rotina está iniciada
    lcd.clear();
    if (rotina_iniciada)
    {
      lcd.setCursor(0, 0);
      lcd.print("Rotina: Iniciada");
      Serial.println("Rotina: Iniciada");
    }
    else
    {
      lcd.setCursor(0, 0);
      lcd.print("Rotina: Parada");
      Serial.println("Rotina: Parada");
    }

    // Exibir o status dos atuadores
    lcd.setCursor(0, 1);
    lcd.print("B_A: ");
    lcd.print(estadoBombaAquario ? "ON" : "OFF");
    
    lcd.setCursor(9, 1);
    lcd.print("B_C: ");
    lcd.print(estadoBombaCisterna ? "ON" : "OFF");

    lcd.setCursor(0, 2);
    lcd.print("Nivel Cisterna:");
    lcd.print(sensorNivelAltoCisterna == 1 ? "Alto" : (sensorNivelBaixoCisterna == 1 ? "Medio" : "Baixo"));

    lcd.setCursor(0, 3);
    lcd.print("Nivel Aquario:");
    lcd.print(sensorNivelAltoAquario == 0 ? "Alto" : (sensorNivelBaixoAquario == 0 ? "Medio" : "Baixo"));
    
    Serial.print("******* B_A:");
    Serial.println(estadoBombaAquario ? "ON" : "OFF");
    Serial.print("******* B_C:");
    Serial.println(estadoBombaCisterna ? "ON" : "OFF");
    Serial.printf("**C_sensor nivel baixo : %d\n", sensorNivelBaixoCisterna);
    Serial.printf("**C_sensor nivel alto: %d\n", sensorNivelAltoCisterna);
    Serial.printf("**A_sensor nivel baixo : %d\n", sensorNivelBaixoAquario);
    Serial.printf("**A_sensor nivel alto: %d\n", sensorNivelAltoAquario);
  }
}

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  // Aquario
  pinMode(nivelAltoAquario, INPUT_PULLUP);
  pinMode(nivelBaixoAquario, INPUT_PULLUP);
  pinMode(valvulaAquario, OUTPUT);
  pinMode(bombaAquario, OUTPUT);
  // tubulacao
  pinMode(valvulaDescarte, OUTPUT);
  pinMode(valvulaHidroponia, OUTPUT);
  // boxWatter
  pinMode(nivelBaixoCisterna, INPUT);
  pinMode(nivelAltoCisterna, INPUT);
  pinMode(bombaCisterna, OUTPUT);

  Serial.println("Desligando atuadores...");
  digitalWrite(bombaCisterna, LOW);
  digitalWrite(valvulaDescarte, LOW);
  digitalWrite(valvulaHidroponia, LOW);
  digitalWrite(valvulaAquario, LOW);
  digitalWrite(bombaAquario, HIGH); //bomba aquario desativa em HIGH
  //myservo.attach(motorServo);
  initWifi();
  if (WiFi.status() == WL_CONNECTED)
  {
    initFirebase();
  } 
}

void loop()
{
  if (!ntp.update())
  {
   ntp.forceUpdate();
  }
  static unsigned long iniciaVerificacao = 0;
  static unsigned long iniciaVerificacaoPH = 0;
  //alimentacao();
  mostrarLCD(digitalRead(nivelBaixoCisterna), digitalRead(nivelAltoCisterna), digitalRead(nivelBaixoAquario), digitalRead(nivelAltoAquario));
  regarPlantas();
  if (millis() - iniciaVerificacao >= 500)
  {
    iniciaVerificacao = millis();
    encherDecantador();
    reabastecerAquario();
  }
  if (millis() - iniciaVerificacaoPH >= 50000) //verifica o ph a cada 50 segundos
  {
    iniciaVerificacaoPH = millis();
    verificaPH(); 
  }
}