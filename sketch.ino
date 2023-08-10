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

LiquidCrystal_I2C lcd(0x27,20,4);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const float gama = 0.7;
const float cons_rl = 50;

bool switchFan = false;
bool switchLamp = false;

DHT dht_1(DHT_PIN, DHT_TYPE);

// Batas sensor akan menyalakan kipas dan lampu
float limitLux = 200;
float limitTemperature = 32;


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
      mqttClient.subscribe("kelompok4/smarthome/switchFan");
      mqttClient.subscribe("kelompok4/smarthome/switchLamp");
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
  if(String(topic) == "kelompok4/smarthome/switchFan"){
    if (messageTemp == "on"){
      switchFan = true;
    } else {
      switchFan = false;
    }
  }
  if(String(topic) == "kelompok4/smarthome/switchLamp"){
    if (messageTemp == "on"){
      switchLamp = true;
    } else {
      switchLamp = false;
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
  mqttClient.setServer(MQTT_Server, MQTT_Port);
  mqttClient.setCallback(callback);

  

  pinMode(ldrPin, INPUT);
  dht_1.begin();
  
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(switch_kipas, OUTPUT);
  digitalWrite(switch_kipas, LOW);

  pinMode(switch_lampu, OUTPUT);
  digitalWrite(switch_lampu, LOW);

}

void loop() {

  while(!mqttClient.connected())
  {
    connect_ke_Broker();   // Try to connect with broker
  }

  mqttClient.loop();


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
    Serial.print("hum: ");
    Serial.println(string_data_hum);
    mqttClient.publish("kelompok4/smarthome/humidity", string_data_hum);


 
  lcd.setCursor(0,0);
  lcd.print("T : ");
  lcd.setCursor(4,0);
  lcd.print(temp);


  
  lcd.setCursor(10,0);
  lcd.print("H : ");
  lcd.setCursor(14,0);
  lcd.print(hum);

  
  lcd.setCursor(0,1);
  lcd.print("Lx : ");
  lcd.setCursor(5,1);
  lcd.print(nilai_lux);

  // Control Temperature dan Lux
  if ((nilai_lux <= limitLux) || (switchLamp)) {
    digitalWrite(switch_lampu, HIGH);
  } else {
    digitalWrite(switch_lampu, LOW);
  }
  Serial.print("Fan:");
  Serial.println(switchFan);
  if ((temp >= limitTemperature)|| (switchFan)) {
    digitalWrite(switch_kipas, HIGH);
  } else {
    digitalWrite(switch_kipas, LOW);
  }


}
