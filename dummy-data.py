#!/usr/bin/env python3
import time
import random
import json
import paho.mqtt.client as mqtt

# MQTT Broker Configuration
MQTT_BROKER = "localhost"               # Change to your broker IP if needed
MQTT_PORT = 1883                        # Default port for Mosquitto
MQTT_USERNAME = "weather_user"
MQTT_PASSWORD = "WeatherWizards"         # Replace with your actual MQTT password

# MQTT Topics (matches your ESP32 structure)
topics = {
    "ens160/aqi": "weather/ens160/aqi",
    "ens160/tvoc": "weather/ens160/tvoc",
    "ens160/eco2": "weather/ens160/eco2",
    "ens160/temperatureF": "weather/ens160/temperatureF",
    "bme280/humidity": "weather/bme280/humidity",
    "bme280/pressure": "weather/bme280/pressure",
    "bme280/altitude": "weather/bme280/altitude",
    "bme680/temperature": "weather/bme680/temperature",
    "bme680/pressure": "weather/bme680/pressure",
    "bme680/humidity": "weather/bme680/humidity",
    "bme680/dewpoint": "weather/bme680/dewpoint",
    "bme680/gas": "weather/bme680/gas",
    "bme680/altitude": "weather/bme680/altitude",
    "light/visible_ir": "weather/light/visible_ir",
    "light/infrared": "weather/light/infrared",
    "lightning/distance": "weather/lightning/distance",
    "lightning/strike": "weather/lightning/strike",
    "summary": "weather/summary"
}

# Setup MQTT client
client = mqtt.Client(client_id="PythonDummyWeatherPublisher")
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

# Optional: Print connection result
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ Connected to MQTT Broker!")
    else:
        print("❌ Connection failed with code", rc)

client.on_connect = on_connect
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_start()

try:
    while True:
        # Generate random dummy values
        data = {
            "ens160": {
                "aqi": random.randint(1, 5),
                "tvoc": random.randint(50, 600),
                "eco2": random.randint(400, 1200),
                "temperatureF": round(65 + random.random() * 15, 2)
            },
            "bme280": {
                "humidity": round(30 + random.random() * 60, 1),
                "pressure": round(990 + random.random() * 30, 1),
                "altitude": round(100 + random.random() * 50, 1)
            },
            "bme680": {
                "temperature": round(15 + random.random() * 15, 2),
                "pressure": round(990 + random.random() * 30, 2),
                "humidity": round(40 + random.random() * 40, 1),
                "dewpoint": round(10 + random.random() * 10, 2),
                "gas": round(1 + random.random() * 5, 2),
                "altitude": round(90 + random.random() * 30, 1)
            },
            "light": {
                "visible_ir": random.randint(100, 1000),
                "infrared": random.randint(50, 800)
            },
            "lightning": {
                "distance": round(random.random() * 40, 1) if random.random() > 0.9 else 0,
                "strike": "1" if random.random() > 0.9 else "0"
            }
        }

        # Publish individual values
        for category, sensors in data.items():
            for key, value in sensors.items():
                topic = topics.get(f"{category}/{key}")
                if topic:
                    client.publish(topic, str(value))

        # Publish combined JSON summary
        summary_data = {
            "temperatureF": data["ens160"]["temperatureF"],
            "humidity": data["bme680"]["humidity"],
            "pressure": data["bme680"]["pressure"],
            "tvoc": data["ens160"]["tvoc"],
            "eco2": data["ens160"]["eco2"],
            "aqi": data["ens160"]["aqi"],
            "light_level": data["light"]["visible_ir"],
            "lightning_distance": data["lightning"]["distance"]
        }
        client.publish(topics["summary"], json.dumps(summary_data))

        # Print to terminal
        print("📡 Published dummy weather data:", json.dumps(summary_data, indent=2))

        # Wait 5 seconds before next publish
        time.sleep(5)

except KeyboardInterrupt:
    print("\n🛑 Stopping publisher...")
    client.loop_stop()
    client.disconnect()
