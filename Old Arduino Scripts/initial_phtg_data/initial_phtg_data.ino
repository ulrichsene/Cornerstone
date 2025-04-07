#include "Adafruit_BME680.h"
 
#define SEALEVELPRESSURE_HPA (1013.25)
 
Adafruit_BME680 bme; // Create a BME680 sensor object for I2C
 
void setup() {
  Serial.begin(115200); // try changing to 115200 (was 9600 initially)
  int len = Serial.availableForWrite();
  while (Serial.availableForWrite() == len) {
      Serial.write('\n');
  }
  Serial.println(F("BME680 test"));
  delay(500);

  // Check if the BME680 sensor is properly initialized
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1); // Stay in a loop if the sensor is not found
  }
 
  // Configure sensor settings
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
}
 
void loop() {
  // Check if the BME680 sensor reading was successful
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
 
  // Display temperature in degrees Celsius
  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" °C");
 
  // Display pressure in hPa (hectopascals)
  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");
 
  // Display humidity in percentage
  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
 
  // Calculate Dew Point
  float dewPoint = bme.temperature - ((100 - bme.humidity) / 5);
  Serial.print("Dew Point = ");
  Serial.print(dewPoint);
  Serial.println(" °C");
 
  // Display gas resistance in KOhms
  Serial.print("Gas = ");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" KOhms");
 
  // Calculate and display approximate altitude
  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
 
  // Print a blank line for better readability
  Serial.println();
 
  // Delay for 2 seconds before the next reading
  delay(2000);
}