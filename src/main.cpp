#include <Arduino.h>
#include <DHT.h>
#include <BH1750.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <Adafruit_Sensor.h>

#define LED_RED 15 
#define LED_ORANGE 2  
#define LED_GREEN 14 
#define DHTPIN 16
#define DHTTYPE DHT11
#define SDA_PIN 21
#define SCL_PIN 22

#define WIFI_SSID "Lab-Eng"
#define WIFI_PASSWORD "Lab-Eng123!"

#define MQTT_BROKER "broker.emqx.io"
#define MQTT_PUBLISH "esp32_kekel/data"
#define MQTT_SUBSCRIBE "esp32_kekel/cmd/#"

WiFiClient espClient;
PubSubClient mqtt(espClient);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter(0x23);

Ticker timerPublish;
char g_szDeviceId[64] = "ESP32_Kekel";
Ticker timerMqtt;
float temperature = 0;
float humidity = 0;
float lightlevel = 0;
const int lux_range = 4000;  
const int temp_range = 5000; 
const int humid_range = 6000;    
unsigned long LuxMillis = 0;
unsigned long TemperatureMillis = 0;
unsigned long HumidityMillis = 0;
       
void WifiConnect(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void OnReadPublish(){
  dht.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE, 0x23, &Wire);
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  lightlevel = lightMeter.readLightLevel();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Light: ");
  Serial.print(lightlevel);
  Serial.println("lx");

  // Buat range antar humid, suhu, cahaya
  if (temperature > 28 && humidity > 80)
  {
    digitalWrite(LED_RED, HIGH);
    Serial.println("LED Merah Hidup");
  }
  else{
    digitalWrite(LED_RED, LOW);
    Serial.println("LED Merah Mati");
  }
  if (temperature > 28 && 60 < humidity < 80)
  {
    digitalWrite(LED_ORANGE, HIGH);
    Serial.println("LED Orange Hidup");
  }
  else
  {
    digitalWrite(LED_ORANGE, LOW);
    Serial.println("LED Orange Mati");
  }
  if (temperature < 28 && humidity < 60)
  {
    digitalWrite(LED_GREEN, HIGH);
    Serial.println("LED Hijau Hidup");
  }
  else{
    digitalWrite(LED_GREEN, LOW);
    Serial.println("LED Hijau Mati");
  }
    if (lightlevel > 400){
    Serial.println("WARNING: Pintu kebuka!");
  }
  else{
    Serial.println("WARNING: Pintu tertutup!");
  }
  if (isnan(humidity) || isnan(temperature)){
    Serial.println("Failed");
    return;
  }
}

void callback(char *topic, byte *payload, unsigned int length){
  Serial.print("Message arrived ");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect(){
  while (!mqtt.connected()){
    Serial.print("MQTT connection...");
    if (mqtt.connect(g_szDeviceId))
    {
      Serial.println("Connected");
      mqtt.subscribe(MQTT_SUBSCRIBE);
    }
    else{
      Serial.print("failed ");
      Serial.print(mqtt.state());
      Serial.println(" try again");
      delay(2000);
    }
  }
}

void setup(){
  Serial.begin(460800);
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE, 0x23, &Wire)){
    Serial.println("Error");}
  WifiConnect();
  mqtt.setServer(MQTT_BROKER, 1883);
  mqtt.setCallback(callback);
  while (!mqtt.connected()){
    String clientId = "ESP32_Kekel";
    if (mqtt.connect(clientId.c_str())){
      Serial.println("connected");}
    else{
      Serial.print("failed with state ");
      Serial.print(mqtt.state());
      delay(2000);
    }
  }

  mqtt.publish(MQTT_PUBLISH, "Hello from ESP32_Kekel");
  mqtt.subscribe(MQTT_SUBSCRIBE);
  dht.begin();

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_ORANGE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  sprintf(g_szDeviceId, "ESP32_%s", WiFi.macAddress().c_str());
  Serial.print(g_szDeviceId);
  Serial.print("Ready");
}

void loop(){
  OnReadPublish();
  if (!mqtt.connected()){
    reconnect();
  }
  unsigned long Millis = millis();
  if (Millis - TemperatureMillis >= temp_range){
    mqtt.publish(MQTT_PUBLISH, String(temperature).c_str());
    TemperatureMillis = Millis;
  }
  if (Millis - HumidityMillis >= humid_range){
    mqtt.publish(MQTT_PUBLISH, String(humidity).c_str());
    HumidityMillis = Millis;
  }
  if (Millis - LuxMillis >= lux_range){
    mqtt.publish(MQTT_PUBLISH, String(lightlevel).c_str());
    LuxMillis = Millis;
  }
  mqtt.loop();
  delay(2000);
}