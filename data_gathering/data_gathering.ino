/*
Sensors used:
Sparkfun Environmental Combo (ENS160 / BME280)
Adafruit BME680
Adafruit LTR-303
Sparkfun AS3935

NOTE: When uploading to board, select "Node32s" as the device
also create a "wifi_creds.h" file to put the wifi ssid and password in
*/

#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include "SparkFun_ENS160.h"
#include "SparkFunBME280.h"
#include "Adafruit_BME680.h"
#include "Adafruit_LTR329_LTR303.h"
#include "SparkFun_AS3935.h"
#include "wifi_creds.h"

#define SEALEVELPRESSURE_HPA (1013.25) // for BME680
// for lightning sensor
#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// define all sensor and mqtt related objects
SparkFun_ENS160 myENS;
BME280 myBME280;
Adafruit_BME680 adaBME; // Create a BME680 sensor object for I2C
Adafruit_LTR329 ltr = Adafruit_LTR329();
SparkFun_AS3935 lightning;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

int ensStatus; // for env combo sensor

// Interrupt pin for lightning detection 
const int lightningInt = 17; 
int spiCS = 2; //SPI chip select pin
// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector. 
int intVal = 0;
int noise = 2; // Value between 1-7 
int disturber = 2; // Value between 1-10

// these variables track how long it has been since each type of data was sampled
unsigned long last_combo_sample = 0;
unsigned long last_bme_sample = 0;
unsigned long last_light_sample = 0;
unsigned long last_lightning_sample = 0;

// wifi credentials from wifi_creds.h file
char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

const char broker[] = "35.193.89.30";
int port = 1883;

void sendMqttData(char topic[], String data) {
  mqttClient.beginMessage(topic);
  mqttClient.print(data);
  mqttClient.endMessage();
}

unsigned long handleComboData(unsigned long last_sample) {
  if (last_sample + 5000 < millis()) { // change to be once every 15 seconds
    // Check if the combo sensor reading was successful
    if (myENS.checkDataStatus()) {
      // send data to mqtt server
      sendMqttData("weather/ens160/aqi", String(myENS.getAQI())); // units: 1-5
      sendMqttData("weather/ens160/tvoc", String(myENS.getTVOC())); // units: ppb
      sendMqttData("weather/ens160/eco2", String(myENS.getECO2())); // units: ppm
    }
    else {
      Serial.println("Failed to read from env combo sensor");
    }
    last_sample += 5000;
  }
  return last_sample;
}

unsigned long handleBMEData(unsigned long last_sample) {
  if (last_sample + 5000 < millis()) { // change to be once every 15 seconds
    // Check if the BME680 sensor reading was successful
    if (adaBME.performReading()) {
      // Calculate Dew Point
      float dewPoint = adaBME.temperature - ((100 - adaBME.humidity) / 5);
      // send data to mqtt server
      // subtract 4 degrees C from temp reading as the sensor reading is 
      //  quite consistently about 4 degrees higher than the actual temp
      sendMqttData("weather/bme680/temperatureC", String(adaBME.temperature - 4.0));
      float degrees_f = ((adaBME.temperature - 4.0) * (9.0 / 5.0)) + 32.0;
      sendMqttData("weather/bme680/temperatureF", String(degrees_f));
      sendMqttData("weather/bme680/pressure", String(adaBME.pressure / 100.0)); // units: hPa
      sendMqttData("weather/bme680/humidity", String(adaBME.humidity)); // units: %
      sendMqttData("weather/bme680/dewpoint", String(dewPoint)); // units: degrees C
      sendMqttData("weather/bme680/gas", String(adaBME.gas_resistance / 1000.0)); // units: KOhms
      sendMqttData("weather/bme680/altitude", String(adaBME.readAltitude(SEALEVELPRESSURE_HPA))); // units: m
    }
    else {
      Serial.println("Failed to read from Adafruit BME680");
    }
    last_sample += 5000;
  }
  return last_sample;
}

unsigned long handleLightData(unsigned long last_sample) {
  if (last_sample + 1000 < millis()) {
    bool valid_light;
    uint16_t visible_plus_ir, infrared;
    if (ltr.newDataAvailable()) {
      valid_light = ltr.readBothChannels(visible_plus_ir, infrared);
      if (valid_light) {
        // send data to mqtt server
        sendMqttData("weather/light/visible_ir", String(visible_plus_ir));
        sendMqttData("weather/light/infrared", String(infrared));
      }
    }
    last_sample += 1000;
  }
  return last_sample;
}

unsigned long handleLightningData(unsigned long last_sample) {
  if (last_sample + 250 < millis()) {
    // Hardware has alerted us to an event, now we read the interrupt register
    if(digitalRead(lightningInt) == HIGH){
      intVal = lightning.readInterruptReg();
      if(intVal == NOISE_INT){
        Serial.println("Noise on lightning sensor");
        // Too much noise? Uncomment the code below, a higher number means better
        // noise rejection.
        lightning.setNoiseLevel(noise); 
        // briefly turn off and back on the built in LED to show that the sensor is reading noise
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else if(intVal == DISTURBER_INT){
        Serial.println("Disturber on lightning sensor");
        // Too many disturbers? Uncomment the code below, a higher number means better
        // disturber rejection.
        lightning.watchdogThreshold(disturber);  
        // briefly turn off and back on the built in LED to show that the sensor is reading distruber
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else if(intVal == LIGHTNING_INT){
        // Lightning! Now how far away is it? Distance estimation takes into
        // account any previously seen events in the last 15 seconds. 
        byte distance = lightning.distanceToStorm(); 
        // send data to mqtt server
        sendMqttData("weather/lightning/distance", String(distance));
      }
    }
    last_sample += 250;
  }
  return last_sample;
}

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Wire.begin(21, 22);
  //////////////////////
  // env combo sensor //
  Serial.println("env combo setup");
  if (!myENS.begin()) {
    Serial.println("Did not begin.");
    while (1)
      ;
  }
  if (myBME280.beginI2C() == false)  //Begin communication over I2C
  {
    Serial.println("The sensor did not respond. Please check wiring.");
    while (1)
      ;  //Freeze
  }
  // Reset the indoor air quality sensor's settings.
  if (myENS.setOperatingMode(SFE_ENS160_RESET))
    Serial.println("Sparkfun BME280 ready.");

  delay(100);

  // Device needs to be set to idle to apply any settings.
  // myENS.setOperatingMode(SFE_ENS160_IDLE);

  // Set to standard operation
  // Others include SFE_ENS160_DEEP_SLEEP and SFE_ENS160_IDLE
  myENS.setOperatingMode(SFE_ENS160_STANDARD);

  // There are four values here:
  // 0 - Operating ok: Standard Operation
  // 1 - Warm-up: occurs for 3 minutes after power-on.
  // 2 - Initial Start-up: Occurs for the first hour of operation.
  //      and only once in sensor's lifetime.
  // 3 - No Valid Output
  ensStatus = myENS.getFlags();
  Serial.print("Gas Sensor Status Flag: ");
  Serial.println(ensStatus);
  
  ///////////////////
  // BME680 sensor //
  Serial.println(F("BME680 setup"));
 
  // Check if the BME680 sensor is properly initialized
  if (!adaBME.begin(0x76)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1); // Stay in a loop if the sensor is not found
  }
 
  // Configure sensor settings
  adaBME.setTemperatureOversampling(BME680_OS_8X);
  adaBME.setHumidityOversampling(BME680_OS_2X);
  adaBME.setPressureOversampling(BME680_OS_4X);
  adaBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
  adaBME.setGasHeater(320, 150); // 320°C for 150 ms

  //////////////////
  // light sensor //
  Serial.println("Adafruit LTR-329 setup");

  if ( ! ltr.begin() ) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }
  Serial.println("Found LTR sensor!");

  ltr.setGain(LTR3XX_GAIN_2);
  Serial.print("Gain : ");
  switch (ltr.getGain()) {
    case LTR3XX_GAIN_1: Serial.println(1); break;
    case LTR3XX_GAIN_2: Serial.println(2); break;
    case LTR3XX_GAIN_4: Serial.println(4); break;
    case LTR3XX_GAIN_8: Serial.println(8); break;
    case LTR3XX_GAIN_48: Serial.println(48); break;
    case LTR3XX_GAIN_96: Serial.println(96); break;
  }

  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  Serial.print("Integration Time (ms): ");
  switch (ltr.getIntegrationTime()) {
    case LTR3XX_INTEGTIME_50: Serial.println(50); break;
    case LTR3XX_INTEGTIME_100: Serial.println(100); break;
    case LTR3XX_INTEGTIME_150: Serial.println(150); break;
    case LTR3XX_INTEGTIME_200: Serial.println(200); break;
    case LTR3XX_INTEGTIME_250: Serial.println(250); break;
    case LTR3XX_INTEGTIME_300: Serial.println(300); break;
    case LTR3XX_INTEGTIME_350: Serial.println(350); break;
    case LTR3XX_INTEGTIME_400: Serial.println(400); break;
  }

  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  Serial.print("Measurement Rate (ms): ");
  switch (ltr.getMeasurementRate()) {
    case LTR3XX_MEASRATE_50: Serial.println(50); break;
    case LTR3XX_MEASRATE_100: Serial.println(100); break;
    case LTR3XX_MEASRATE_200: Serial.println(200); break;
    case LTR3XX_MEASRATE_500: Serial.println(500); break;
    case LTR3XX_MEASRATE_1000: Serial.println(1000); break;
    case LTR3XX_MEASRATE_2000: Serial.println(2000); break;
  }

  //////////////////////
  // lightning sensor //
  // When lightning is detected the interrupt pin goes HIGH.
  pinMode(lightningInt, INPUT); 
  Serial.println("AS3935 lightning detector setup"); 

  SPI.begin(); 
  pinMode(LED_BUILTIN, OUTPUT);

  if( !lightning.beginSPI(spiCS, 2000000) ){ 
    Serial.println ("Lightning detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("Lightning detector ready!");
  // not sure why but the above if statement turns on the built in LED, so we turn it back off afterwards
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  // The lightning detector defaults to an indoor setting at 
  // the cost of less sensitivity, when using the system outdoors 
  // uncomment the following line:
  //lightning.setIndoorOutdoor(OUTDOOR); 

  /////////////////////////
  // network connections //
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, pass); // Connect to the Wi-Fi network
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to the network!");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("Connected to the MQTT broker!");
  Serial.println();
  // turn on LED to show visually that the system has finished setup
  digitalWrite(LED_BUILTIN, HIGH);

  // set initial data read times to be when the setup is finished
  unsigned long start_time = millis();
  last_combo_sample = start_time;
  last_bme_sample = start_time;
  last_light_sample = start_time;
  last_lightning_sample = start_time;
  Serial.println("done with setup");
}

void loop() {
  // env combo sensor
  last_combo_sample = handleComboData(last_combo_sample);

  // BME680 sensor
  last_bme_sample = handleBMEData(last_bme_sample);

  // light sensor
  last_light_sample = handleLightData(last_light_sample);

  // lightning sensor
  last_lightning_sample = handleLightningData(last_lightning_sample);
}