#include <WiFi.h>

#include <PubSubClient.h>

#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define ssid "Wokwi-GUEST"
#define password ""

#define ldrPin 34

#define DHT_PIN 2
#define DHT_TYPE DHT22

#define switch_kipas 15
#define switch_lampu 4

char *MQTT_Server = "broker.hivemq.com";;
int  MQTT_Port = 1883;

LiquidCrystal_I2C lcd(0x27,16,2);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const float gama = 0.7;
const float cons_rl = 50;

DHT dht_1(DHT_PIN, DHT_TYPE);

// Batas sensor akan menyalakan kipas dan lampu
float limitLux = 200;
float limitTemperature = 32;

void Initial_Setup_MQTT()
{
  mqttClient.setServer(MQTT_Server, MQTT_Port);
  mqttClient.setCallback(callback);
}

void connect_ke_Broker()
{
  Serial.println("Menyambungkan ke Broker");
  while(!mqttClient.connected())
  {
    Serial.println("Menyambungkan ke Broker");
    String clientID = "ESP32_TRIAL";
    clientID += String(random(0xffff,HEX));

    if(mqttClient.connect(clientID.c_str()))
    {
      Serial.println("Broker Tersambung");
      mqttClient.subscribe("kelompok4/smarthome/limitLux");
      mqttClient.subscribe("kelompok4/smarthome/limitTemperature");
    }
  }
}

void callback(char* topic, byte* message, unsigned int length)
{
  Serial.print("Callback - ");
  Serial.print("Message:");
  String messageTemp;
  for(int i = 0 ; i < length ; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  if(String(topic) == "kelompok4/smarthome/limitLux"){
    if (messageTemp.toInt()!=0){
      limitLux = messageTemp.toFloat();
    }
  }
  if(String(topic) == "kelompok4/smarthome/limitTemperature"){
    if (messageTemp.toInt()!=0){
      limitTemperature = messageTemp.toFloat();
    }
  }
}

void cek_Internet()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("WiFi Belum Connect");
  }
  Serial.println("WiFi Connected");
}
void setup()
{
  Serial.begin(115200);

  Serial.println("PERANGKAT ON");

  delay(2000);

  cek_Internet();
  Initial_Setup_MQTT();

  

  pinMode(ldrPin, INPUT);
  dht_1.begin();
  lcd.init();

  pinMode(switch_kipas, OUTPUT);
  digitalWrite(switch_kipas, LOW);

}

void loop()
{


  while(!mqttClient.connected())
  {
    connect_ke_Broker();   // Try to connect with broker
  }

  //else
  //{
    int analog = analogRead(ldrPin);

    Serial.print("Nilai Analog: ");
    Serial.print(analog);
  
    int konv_analog = map(analog, 4095,0,1023,0);

    float tegangan = konv_analog / 1024.*5;
    float nilai_R_LDR = 2000 * tegangan / (1-tegangan/5);
    float nilai_lux = pow(cons_rl * 1e3 * pow(10,gama)/nilai_R_LDR,(1/gama));
  
    Serial.print("\t Nilai Lux: ");
    Serial.println(nilai_lux);
    
    char string_data_lux[8];
    dtostrf(nilai_lux, 1, 2, string_data_lux);  //Convert float to String
    Serial.print("nilai Lux: ");
    Serial.println(string_data_lux);
    mqttClient.publish("kelompok4/smarthome/nilai lux", string_data_lux);


    float temp = dht_1.readTemperature(); // Baca suhu dalam derajat Celsius
    float hum = dht_1.readHumidity();

    char string_data_temp[8];
    dtostrf(temp, 1, 2, string_data_temp);  //Convert float to String
    Serial.print("T: ");
    Serial.println(string_data_temp);
    mqttClient.publish("kelompok4/smarthome/temperature", string_data_temp);
    
    char string_data_hum[8];
    dtostrf(hum, 1, 2, string_data_hum);  //Convert float to String
    Serial.print("Temperature: ");
    Serial.println(string_data_hum);
    mqttClient.publish("kelompok4/smarthome/humidity", string_data_hum);
  //}

  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.setCursor(2,0);
  lcd.print(temp);

  lcd.backlight();
  lcd.setCursor(8,0);
  lcd.print("Lx:");
  lcd.setCursor(11,0);
  lcd.print(nilai_lux);

  // Control Temperature dan Lux
  if (nilai_lux <= limitLux) {
    digitalWrite(switch_lampu, HIGH);
  } else {
    digitalWrite(switch_lampu, LOW);
  }

  if (temp >= limitTemperature) {
    digitalWrite(switch_kipas, HIGH);
  } else {
    digitalWrite(switch_kipas, LOW);
  }


}