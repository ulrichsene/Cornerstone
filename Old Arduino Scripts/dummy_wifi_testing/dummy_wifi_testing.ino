#include "wifi_creds.h"
#include <WiFi.h>
#include <WebServer.h> // Or ESPAsyncWebServer.h

const char* ssid = wifi_ssid;
const char* password = wifi_passwd;

//3. Connect to Wi-Fi:**

//   In the `setup()` function, initialize the serial communication (for debugging) and then connect to the Wi-Fi network:
//c++
// void setup() {
//   Serial.begin(115200);
//   delay(1000); // Allow time for serial monitor to open

//   WiFi.begin(ssid, password); // Connect to the Wi-Fi network

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("WiFi connected");
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());
// }

// Create a web server object
WebServer server(80); // Or ESPAsyncWebServer server;

void setup() {
  Serial.begin(115200);
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Define the web server routes
  server.on("/", []() {
    server.send(200, "text/html", "<h1>Hello, World!</h1>");
  });

  server.begin();
}

void loop() {
  // Handle incoming web requests
  server.handleClient();
}