#include <EEPROM.h> // EEPROM (memory) library.
#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h> // SPI device communication library.
const char* ssid = ""; //Name of the Wi-Fi used.
const char* password = ""; // Password of the Wi-Fi used.
const char* subtopic = ""; //subscribe topic name
const char* pubtopic = ""; // publish topic name
const char* mqtt_user = ""; // MQTT Username.
const char* mqtt_pass = ""; // MQTT Password.
int mqtt_port = ; //MQTT Port Address
const char* mqtt_server = ""; //MQTT server name
WiFiClient espClient;
PubSubClient client(espClient);


#define ONE_WIRE_BUS 12
#define DHTTYPE DHT22 
#define DHTPIN 13 
DFRobot_ESP_PH ph;
#define ESPADC 4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300 //the esp voltage supply value
#define PH_PIN 35    //the esp gpio data pin number
float voltage, phValue, temperature = 25;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float Celcius=0;
float Fahrenheit=0;


void setup()
{ 
  Serial.begin(115200);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  SPI.begin(); // Initiates SPI connection between RFID module and Arduino.
  sensors.begin();
  EEPROM.begin(32);//needed to permit storage of calibration value in eeprom
  Serial.println(F("DHTxx test!"));
  ph.begin();
  dht.begin();
}

void setup_wifi() 
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(subtopic);
  Serial.print("] ");
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement..
      client.subscribe(subtopic);
    } else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(50);
    }

   }
}

void loop()
{
  if (!client.connected()) 
  {
    reconnect();
  }
   pH_value();
   dht22();
   water_temp();
   client.loop();
}

void pH_value()
{
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) //time interval: 1s
  {
    timepoint = millis();
    //voltage = rawPinValue / esp32ADC * esp32Vin
    voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
    //Serial.print("voltage:");
    //Serial.println(voltage, 4);
    
    //temperature = readTemperature();  // read your temperature sensor to execute temperature compensation
    //Serial.print("temperature:");
    //Serial.print(temperature, 1);
    //Serial.println("^C");

    phValue = ph.readPH(voltage, temperature); // convert voltage to pH with temperature compensation
    //Serial.print("pH:");
    Serial.println(phValue, 4);
  }
  ph.calibration(voltage, temperature); // calibration process by Serail CMD
  delay(2000);
  client.publish(pubtopic, String(phValue).c_str(), true);
  client.subscribe(subtopic);
}

void dht22()
{
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.println(h);
  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.println(F("°F "));
  Serial.print(F(" Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  client.publish(pubtopic, String(h).c_str(), true);  
  client.publish(pubtopic, String(t).c_str(), true); 
  client.publish(pubtopic, String(f).c_str(), true);  
  client.publish(pubtopic, String(hic).c_str(), true); 
  client.publish(pubtopic, String(hif).c_str(), true);
  client.subscribe(subtopic); 
}


void water_temp()
{
  sensors.requestTemperatures(); 
  Celcius=sensors.getTempCByIndex(0);
  Fahrenheit=sensors.toFahrenheit(Celcius);
  Serial.print("water_temp: ");
  Serial.print(Celcius);
  Serial.println("°C");
  Serial.print("water_temp: ");
  Serial.print(Fahrenheit);
  Serial.println("°F");
  delay(1000);
  client.publish(pubtopic, String(Celcius).c_str(), true);  
  client.publish(pubtopic, String(Fahrenheit).c_str(), true);
  client.subscribe(subtopic); 
}
