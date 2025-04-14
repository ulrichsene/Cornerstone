// #include <ArduinoMqttClient.h>
// #include <WebServer.h> // Or ESPAsyncWebServer.h
// //#include <WiFiNINA.h>
// #include <WiFi.h>
// #include "wifi_creds.h"

// WiFiClient wifiClient;
// MqttClient mqttClient(wifiClient);
// const char broker[] = "test.mosquitto.org";
// int        port     = 1883;


// const char* ssid = WIFI_SSID;
// const char* password = WIFI_PASS;

// //Connect to Wi-Fi:

// //   In the `setup()` function, initialize the serial communication (for debugging) and then connect to the Wi-Fi network:
// //c++
// // void setup() {
// //   Serial.begin(115200);
// //   delay(1000); // Allow time for serial monitor to open

// //   WiFi.begin(ssid, password); // Connect to the Wi-Fi network

// //   while (WiFi.status() != WL_CONNECTED) {
// //     delay(500);
// //     Serial.print(".");
// //   }

// //   Serial.println("WiFi connected");
// //   Serial.print("IP address: ");
// //   Serial.println(WiFi.localIP());
// // }

// // Create a web server object
// WebServer server(80); // Or ESPAsyncWebServer server;

// void setup() {
//   Serial.begin(115200);
//   // Connect to Wi-Fi
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("");
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(WiFi.localIP());
//   if (!mqttClient.connect(broker, port)) {
//     Serial.print("MQTT connection failed! Error code = ");
//     Serial.println(mqttClient.connectError());

//     while (1);
//   }
//   // Define the web server routes
//   server.on("/", []() {
//     server.send(200, "text/html", "<h1>Hello, World!</h1>");
//   });

//   server.begin();
// }

// void loop() {
//   // Handle incoming web requests
//   server.handleClient();
// }