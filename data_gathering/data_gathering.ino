/*

Sensors used:
Sparkfun Environmental Combo (ENS160 / BME280)
Adafruit BME680
Adafruit LTR-303
Sparkfun AS3935

NOTE: When uploading to board, select "Node32s" as the device
*/

#include <SPI.h>
//#include <WiFi.h>
#include <Wire.h>
#include "SparkFun_ENS160.h"
#include "SparkFunBME280.h"
#include "Adafruit_BME680.h"
#include "Adafruit_LTR329_LTR303.h"
#include "SparkFun_AS3935.h"
//#include "wifi_creds.h"

#define SEALEVELPRESSURE_HPA (1013.25) // for BME680
// for lightning sensor
#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

// define all sensor objects
SparkFun_ENS160 myENS;
BME280 myBME280;
Adafruit_BME680 adaBME; // Create a BME680 sensor object for I2C
Adafruit_LTR329 ltr = Adafruit_LTR329();
SparkFun_AS3935 lightning;

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

unsigned long handleComboData(unsigned long last_sample) {
  if (last_sample + 5000 < millis()) { // change to be once every 15 seconds
    // Check if the combo sensor reading was successful
    if (myENS.checkDataStatus()) {
      Serial.print("Air Quality Index (1-5) : ");
      Serial.println(myENS.getAQI());

      Serial.print("Total Volatile Organic Compounds: ");
      Serial.print(myENS.getTVOC());
      Serial.println("ppb");

      Serial.print("CO2 concentration: ");
      Serial.print(myENS.getECO2());
      Serial.println("ppm");

      Serial.print("Humidity: ");
      Serial.print(myBME280.readFloatHumidity(), 0);
      Serial.println("RH%");

      Serial.print("Pressure: ");
      Serial.print(myBME280.readFloatPressure(), 0);
      Serial.println("Pa");

      Serial.print("Alt: ");
      //Serial.print(myBME280.readFloatAltitudeMeters(), 1);
      //Serial.println("meters");
      Serial.print(myBME280.readFloatAltitudeFeet(), 1);
      Serial.println("feet");

      Serial.print("Temp: ");
      //Serial.print(myBME280.readTempC(), 2);
      //Serial.println(" degC");
      Serial.print(myBME280.readTempF(), 2);
      Serial.println(" degF");
    }
    else {
      Serial.println("Failed to read from env combo sensor");
    }
    // Print a blank line for better readability
    Serial.println();
    last_sample += 5000;
  }
  return last_sample;
}

unsigned long handleBMEData(unsigned long last_sample) {
  if (last_sample + 5000 < millis()) { // change to be once every 15 seconds
    // Check if the BME680 sensor reading was successful
    if (adaBME.performReading()) {
      // Display temperature in degrees Celsius
      Serial.print("Temperature = ");
      Serial.print(adaBME.temperature);
      Serial.println(" °C");
    
      // Display pressure in hPa (hectopascals)
      Serial.print("Pressure = ");
      Serial.print(adaBME.pressure / 100.0);
      Serial.println(" hPa");
    
      // Display humidity in percentage
      Serial.print("Humidity = ");
      Serial.print(adaBME.humidity);
      Serial.println(" %");
    
      // Calculate Dew Point
      float dewPoint = adaBME.temperature - ((100 - adaBME.humidity) / 5);
      Serial.print("Dew Point = ");
      Serial.print(dewPoint);
      Serial.println(" °C");
    
      // Display gas resistance in KOhms
      Serial.print("Gas = ");
      Serial.print(adaBME.gas_resistance / 1000.0);
      Serial.println(" KOhms");
    
      // Calculate and display approximate altitude
      Serial.print("Approx. Altitude = ");
      Serial.print(adaBME.readAltitude(SEALEVELPRESSURE_HPA));
      Serial.println(" m");
    }
    else {
      Serial.println("Failed to read from Adafruit BME680");
    }
    // Print a blank line for better readability
    Serial.println();
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
        Serial.print("CH0 Visible + IR: ");
        Serial.print(visible_plus_ir);
        Serial.print("\t\tCH1 Infrared: ");
        Serial.println(infrared);
      }
    }
    // Print a blank line for better readability
    Serial.println();
    last_sample += 1000;
  }
  return last_sample;
}

unsigned long handleLightningData(unsigned long last_sample) {
  if (last_sample + 200 < millis()) {
    // Hardware has alerted us to an event, now we read the interrupt register
    if(digitalRead(lightningInt) == HIGH){
      intVal = lightning.readInterruptReg();
      if(intVal == NOISE_INT){
        Serial.println("Noise."); 
        Serial.println();
        // Too much noise? Uncomment the code below, a higher number means better
        // noise rejection.
        //lightning.setNoiseLevel(noise); 
      }
      else if(intVal == DISTURBER_INT){
        Serial.println("Disturber."); 
        Serial.println();
        // Too many disturbers? Uncomment the code below, a higher number means better
        // disturber rejection.
        //lightning.watchdogThreshold(disturber);  
      }
      else if(intVal == LIGHTNING_INT){
        Serial.println("Lightning Strike Detected!"); 
        // Lightning! Now how far away is it? Distance estimation takes into
        // account any previously seen events in the last 15 seconds. 
        byte distance = lightning.distanceToStorm(); 
        Serial.print("Approximately: "); 
        Serial.print(distance); 
        Serial.println("km away!"); 
        Serial.println();
      }
    }
    last_sample += 200;
  }
  return last_sample;
}

void setup() {
  
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  bool light_status = false;
  while (!Serial){
    if( light_status ) {
          digitalWrite(LED_BUILTIN, LOW);
    } else {
          digitalWrite(LED_BUILTIN, HIGH);
    }
      light_status = !light_status;
    delay(500);
  }

  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  
  Wire.begin(21, 22);
  //////////////////////
  // env combo sensor //
  Serial.println("env combo test");
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
  //                                              and only once in sensor's lifetime.
  // 3 - No Valid Output
  ensStatus = myENS.getFlags();
  Serial.print("Gas Sensor Status Flag: ");
  Serial.println(ensStatus);
  
  ///////////////////
  // BME680 sensor //
  Serial.println(F("BME680 test"));
 
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
  Serial.println("Adafruit LTR-329 advanced test");

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

  Serial.begin(115200); 
  Serial.println("AS3935 Franklin Lightning Detector"); 

  SPI.begin(); 

  if( !lightning.beginSPI(spiCS, 2000000) ){ 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  else
    Serial.println("Schmow-ZoW, Lightning Detector Ready!");

  // The lightning detector defaults to an indoor setting at 
  // the cost of less sensitivity, if you plan on using this outdoors 
  // uncomment the following line:
  //lightning.setIndoorOutdoor(OUTDOOR); 

  Serial.println("done with setup");
  // delay to allow combo sensor to actually finish getting ready
  delay(500);
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
 
} how to make this a listener for mqtt // publish-test-data.js
require('dotenv').config();
const mqtt = require('mqtt');

const client = mqtt.connect('mqtt://localhost:5173/mqtt', {
  clientId: weather_wizards_${Math.random().toString(16).substring(2, 10)},
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
  username: 'weather_user',
  password: process.env.MQTT_PASSWORD,
  rejectUnauthorized: false 
});

client.on('connect', () => {
  console.log('Connected to MQTT broker');
  
  // Publish random weather data every 5 seconds
  setInterval(() => {
    const temperature = (15 + Math.random() * 15).toFixed(1);
    const humidity = (40 + Math.random() * 40).toFixed(1);
    const pressure = (1000 + Math.random() * 30).toFixed(1);
    const airQuality = (Math.random() * 100).toFixed(1);
    const lightLevel = (Math.random() * 100).toFixed(1);
    const lightning = (Math.random() > 0.9 ? Math.random() * 10 : 0).toFixed(1);
    
    console.log(Publishing - Temp: ${temperature}°C, Humidity: ${humidity}%);
    
    client.publish('weather/temperature', temperature);
    client.publish('weather/humidity', humidity);
    client.publish('weather/pressure', pressure);
    client.publish('weather/air-quality', airQuality);
    client.publish('weather/light-level', lightLevel);
    client.publish('weather/lightning', lightning);
  }, 5000);
});

client.on('error', (err: Error) => {
  console.error('MQTT Error:', err.message);
});