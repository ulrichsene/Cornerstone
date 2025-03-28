#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial Monitor
  Serial.println("\nI2C Scanner");

  Wire.begin(26, 18);  // Change to (26, 35) if needed
}

void loop() {
  Serial.println("Scanning...");
  byte count = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(address, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("No I2C devices found. Check wiring!");
  } else {
    Serial.print("Total devices found: ");
    Serial.println(count);
  }

  delay(2000);  // Scan every 2 seconds
}
