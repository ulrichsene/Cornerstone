/*
Sensors used:
Sparkfun Environmental Combo (ENS160 / BME280)
Adafruit BME680
Adafruit LTR-303
Sparkfun AS3935

NOTE: When uploading to board, select "Node32s" as the device
*/

#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#include "SparkFun_ENS160.h"
#include "SparkFunBME280.h"
#include "Adafruit_BME680.h"
#include "Adafruit_LTR329_LTR303.h"
#include "SparkFun_AS3935.h"
#include "wifi_creds.h"

// -------------------------
// WiFi & MQTT Configuration
// -------------------------
const char* ssid         = WIFI_SSID;
const char* wifiPassword = WIFI_PASSWORD;

const char* mqtt_server  = MQTT_SERVER;
const int   mqtt_port    = MQTT_PORT;
const char* mqtt_user    = MQTT_USER;
const char* mqtt_pass    = MQTT_PASSWORD;


WiFiClient espClient;
PubSubClient client(espClient);

// -------------------------
// Sensor Objects & Settings
// -------------------------
SparkFun_ENS160 myENS;
BME280 myBME280;
Adafruit_BME680 adaBME;  // BME680 sensor object for I2C
Adafruit_LTR329 ltr = Adafruit_LTR329();
SparkFun_AS3935 lightning;

int ensStatus;  // for environment combo sensor

// Lightning sensor definitions
#define LIGHTNING_INT 17  // Use pin 17 for lightning interrupt
int spiCS = 2;            // SPI chip select pin for lightning sensor

// Timing variables for sensor sampling
unsigned long last_combo_sample     = 0;
unsigned long last_bme_sample       = 0;
unsigned long last_light_sample     = 0;
unsigned long last_lightning_sample = 0;

// Variables for lightning sensor events
int intVal   = 0;
int noise    = 2;   // Value between 1-7 for noise rejection
int disturber = 2;  // Value between 1-10 for disturber rejection

// For BME680 altitude calculation
#define SEALEVELPRESSURE_HPA (1013.25)

// Lightning sensor interrupt flags
#define INDOOR        0x12 
#define OUTDOOR       0xE
#define LIGHTNING_FLAG 0x08
#define DISTURBER_FLAG 0x04
#define NOISE_FLAG     0x01

// -------------------------
// Function Prototypes
// -------------------------
unsigned long handleComboData(unsigned long last_sample);
unsigned long handleBMEData(unsigned long last_sample);
unsigned long handleLightData(unsigned long last_sample);
unsigned long handleLightningData(unsigned long last_sample);
void connectToWiFi();
void reconnectMQTT();

// -------------------------
// WiFi & MQTT Functions
// -------------------------
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Connect with a unique client ID
    if (client.connect("ESP32_WeatherClient", mqtt_user, mqtt_pass)) {
      Serial.println(" connected!");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

// -------------------------
// Sensor Data Handlers
// -------------------------
unsigned long handleComboData(unsigned long last_sample) {
  // Sample approximately every 5 seconds
  if (last_sample + 5000 < millis()) {
    if (myENS.checkDataStatus()) {
      int aqi      = myENS.getAQI();
      long tvoc    = myENS.getTVOC();
      long eco2    = myENS.getECO2();
      float humidity   = myBME280.readFloatHumidity();
      float pressure   = myBME280.readFloatPressure();
      float altitude   = myBME280.readFloatAltitudeFeet();
      float tempF      = myBME280.readTempF();

      Serial.print("Air Quality Index (1-5) : ");
      Serial.println(aqi);
      Serial.print("Total Volatile Organic Compounds: ");
      Serial.print(tvoc);
      Serial.println(" ppb");
      Serial.print("CO2 concentration: ");
      Serial.print(eco2);
      Serial.println(" ppm");
      Serial.print("Humidity: ");
      Serial.print(humidity, 0);
      Serial.println(" RH%");
      Serial.print("Pressure: ");
      Serial.print(pressure, 0);
      Serial.println(" Pa");
      Serial.print("Altitude (feet): ");
      Serial.print(altitude, 1);
      Serial.println(" feet");
      Serial.print("Temperature (F): ");
      Serial.print(tempF, 2);
      Serial.println(" degF");
      Serial.println();

      // Publish data to MQTT topics
      client.publish("weather/aqi",         String(aqi).c_str());
      client.publish("weather/tvoc",        String(tvoc).c_str());
      client.publish("weather/eco2",        String(eco2).c_str());
      client.publish("weather/humidity",    String(humidity, 0).c_str());
      client.publish("weather/pressure",    String(pressure, 0).c_str());
      client.publish("weather/altitude",    String(altitude, 1).c_str());
      client.publish("weather/temperatureF",String(tempF, 2).c_str());
    }
    else {
      Serial.println("Failed to read from env combo sensor");
    }
    last_sample = millis();
  }
  return last_sample;
}

unsigned long handleBMEData(unsigned long last_sample) {
  // Sample approximately every 5 seconds
  if (last_sample + 5000 < millis()) {
    if (adaBME.performReading()) {
      float temp    = adaBME.temperature;
      float pres    = adaBME.pressure / 100.0;
      float hum     = adaBME.humidity;
      float dewPoint= temp - ((100 - hum) / 5);
      float gas     = adaBME.gas_resistance / 1000.0;
      float alt     = adaBME.readAltitude(SEALEVELPRESSURE_HPA);

      Serial.print("BME680 Temperature = ");
      Serial.print(temp);
      Serial.println(" °C");
      Serial.print("BME680 Pressure = ");
      Serial.print(pres);
      Serial.println(" hPa");
      Serial.print("BME680 Humidity = ");
      Serial.print(hum);
      Serial.println(" %");
      Serial.print("BME680 Dew Point = ");
      Serial.print(dewPoint);
      Serial.println(" °C");
      Serial.print("BME680 Gas = ");
      Serial.print(gas);
      Serial.println(" KOhms");
      Serial.print("BME680 Approx. Altitude = ");
      Serial.print(alt);
      Serial.println(" m");
      Serial.println();

      // Publish data to MQTT topics
      client.publish("weather/bme680/temperature", String(temp).c_str());
      client.publish("weather/bme680/pressure",    String(pres).c_str());
      client.publish("weather/bme680/humidity",    String(hum).c_str());
      client.publish("weather/bme680/dewpoint",    String(dewPoint).c_str());
      client.publish("weather/bme680/gas",         String(gas).c_str());
      client.publish("weather/bme680/altitude",    String(alt).c_str());
    }
    else {
      Serial.println("Failed to read from Adafruit BME680");
    }
    last_sample = millis();
  }
  return last_sample;
}

unsigned long handleLightData(unsigned long last_sample) {
  // Sample approximately every 1 second
  if (last_sample + 1000 < millis()) {
    if (ltr.newDataAvailable()) {
      uint16_t visible_plus_ir, infrared;
      bool valid = ltr.readBothChannels(visible_plus_ir, infrared);
      if (valid) {
        Serial.print("CH0 Visible + IR: ");
        Serial.print(visible_plus_ir);
        Serial.print("\tCH1 Infrared: ");
        Serial.println(infrared);
        // Publish light sensor data
        client.publish("weather/light/visible_ir", String(visible_plus_ir).c_str());
        client.publish("weather/light/infrared",  String(infrared).c_str());
      }
    }
    Serial.println();
    last_sample = millis();
  }
  return last_sample;
}

unsigned long handleLightningData(unsigned long last_sample) {
  // Sample approximately every 200ms
  if (last_sample + 200 < millis()) {
    // Check if an interrupt event has been signaled
    if (digitalRead(LIGHTNING_INT) == HIGH) {
      intVal = lightning.readInterruptReg();
      if (intVal == NOISE_FLAG) {
        Serial.println("Lightning Sensor: Noise.");
      }
      else if (intVal == DISTURBER_FLAG) {
        Serial.println("Lightning Sensor: Disturber.");
      }
      else if (intVal == LIGHTNING_FLAG) {
        Serial.println("Lightning Strike Detected!");
        byte distance = lightning.distanceToStorm();
        Serial.print("Approximately: ");
        Serial.print(distance);
        Serial.println(" km away!");
        // Publish lightning data. A nonzero value here indicates a strike event.
        client.publish("weather/lightning/distance", String(distance).c_str());
        client.publish("weather/lightning/strike", "1");
      }
      Serial.println();
    }
    last_sample = millis();
  }
  return last_sample;
}

// -------------------------
// Setup & Main Loop
// -------------------------
void setup() {
  Serial.begin(115200);

  // Optional: Blink the onboard LED to indicate startup
  pinMode(LED_BUILTIN, OUTPUT);
  bool ledState = false;
  while (!Serial) {
    digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
    ledState = !ledState;
    delay(500);
  }
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);

  // Initialize I2C bus on pins 21 (SDA) and 22 (SCL)
  Wire.begin(21, 22);

  // --------- Environmental Combo Sensor (ENS160 / BME280) ---------
  Serial.println("Initializing Environmental Combo Sensor");
  if (!myENS.begin()) {
    Serial.println("Failed to begin ENS160.");
    while (1);
  }
  if (!myBME280.beginI2C()) {
    Serial.println("BME280 did not respond. Check wiring.");
    while (1);
  }
  // Reset and then set to standard operating mode for the ENS160
  if (myENS.setOperatingMode(SFE_ENS160_RESET))
    Serial.println("ENS160 Reset.");
  delay(100);
  myENS.setOperatingMode(SFE_ENS160_STANDARD);
  ensStatus = myENS.getFlags();
  Serial.print("Gas Sensor Status Flag: ");
  Serial.println(ensStatus);

  // --------- BME680 Sensor ---------
  Serial.println(F("Initializing BME680"));
  if (!adaBME.begin(0x76)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
  adaBME.setTemperatureOversampling(BME680_OS_8X);
  adaBME.setHumidityOversampling(BME680_OS_2X);
  adaBME.setPressureOversampling(BME680_OS_4X);
  adaBME.setIIRFilterSize(BME680_FILTER_SIZE_3);
  adaBME.setGasHeater(320, 150);

  // --------- LTR-329 Light Sensor ---------
  Serial.println("Initializing LTR-329 Light Sensor");
  if (!ltr.begin()) {
    Serial.println("Couldn't find LTR sensor!");
    while (1) delay(10);
  }
  Serial.println("LTR sensor found.");
  ltr.setGain(LTR3XX_GAIN_2);
  Serial.print("Gain: ");
  switch (ltr.getGain()) {
    case LTR3XX_GAIN_1:  Serial.println(1); break;
    case LTR3XX_GAIN_2:  Serial.println(2); break;
    case LTR3XX_GAIN_4:  Serial.println(4); break;
    case LTR3XX_GAIN_8:  Serial.println(8); break;
    case LTR3XX_GAIN_48: Serial.println(48); break;
    case LTR3XX_GAIN_96: Serial.println(96); break;
    default:             Serial.println("Unknown"); break;
  }
  ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
  Serial.print("Integration Time (ms): ");
  switch (ltr.getIntegrationTime()) {
    case LTR3XX_INTEGTIME_50:  Serial.println(50); break;
    case LTR3XX_INTEGTIME_100: Serial.println(100); break;
    case LTR3XX_INTEGTIME_150: Serial.println(150); break;
    case LTR3XX_INTEGTIME_200: Serial.println(200); break;
    case LTR3XX_INTEGTIME_250: Serial.println(250); break;
    case LTR3XX_INTEGTIME_300: Serial.println(300); break;
    case LTR3XX_INTEGTIME_350: Serial.println(350); break;
    case LTR3XX_INTEGTIME_400: Serial.println(400); break;
    default:                   Serial.println("Unknown"); break;
  }
  ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
  Serial.print("Measurement Rate (ms): ");
  switch (ltr.getMeasurementRate()) {
    case LTR3XX_MEASRATE_50:   Serial.println(50); break;
    case LTR3XX_MEASRATE_100:  Serial.println(100); break;
    case LTR3XX_MEASRATE_200:  Serial.println(200); break;
    case LTR3XX_MEASRATE_500:  Serial.println(500); break;
    case LTR3XX_MEASRATE_1000: Serial.println(1000); break;
    case LTR3XX_MEASRATE_2000: Serial.println(2000); break;
    default:                   Serial.println("Unknown"); break;
  }

  // --------- AS3935 Lightning Sensor ---------
  pinMode(LIGHTNING_INT, INPUT);
  Serial.println("Initializing AS3935 Lightning Sensor");
  SPI.begin();
  if (!lightning.beginSPI(spiCS, 2000000)) {
    Serial.println("Lightning Detector did not start, freezing!");
    while (1);
  }
  else {
    Serial.println("Lightning Detector Ready!");
  }
  // If using the sensor outdoors, uncomment the following line:
  // lightning.setIndoorOutdoor(OUTDOOR);

  // --------- Connect to WiFi & MQTT ---------
  connectToWiFi();
  client.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();

  Serial.println("Setup complete. Starting sensor readings...");
  delay(500);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  last_combo_sample     = handleComboData(last_combo_sample);
  last_bme_sample       = handleBMEData(last_bme_sample);
  last_light_sample     = handleLightData(last_light_sample);
  last_lightning_sample = handleLightningData(last_lightning_sample);
}
