#!/usr/bin/env python3
"""
Weather MQTT to InfluxDB Bridge
-------------------------------
Subscribes to weather MQTT topics and writes the data to InfluxDB.
"""
import json
import time
import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

# MQTT Configuration
MQTT_BROKER = "35.193.89.30"
MQTT_PORT = 1883
MQTT_USERNAME = "user"
MQTT_PASSWORD = "secret"
MQTT_CLIENT_ID = "bridge"

# Topics to subscribe to
WEATHER_TOPIC = "weather/#"  # Wildcard to subscribe to all weather topics

# InfluxDB Configuration
INFLUXDB_HOST = "localhost"
INFLUXDB_PORT = 8086
INFLUXDB_USER = "user"
INFLUXDB_PASSWORD = "secret"
INFLUXDB_DATABASE = "weather"

# Mapping of topics to measurements
TOPIC_MAPPING = {
    "weather/ens160/aqi": {"measurement": "air_quality"},
    "weather/ens160/tvoc": {"measurement": "tvoc"},
    "weather/ens160/eco2": {"measurement": "co2_concentration"},
    "weather/bme680/temperatureF": {"measurement": "temperature_f"},
    "weather/bme680/temperatureC": {"measurement": "temperature_c"},
    "weather/bme680/pressure": {"measurement": "pressure"},
    "weather/bme680/humidity": {"measurement": "humidity"},
    "weather/bme680/dewpoint": {"measurement": "dewpoint"},
    "weather/bme680/gas": {"measurement": "gas"},
    "weather/bme680/altitude": {"measurement": "altitude"},
    "weather/light/visible_ir": {"measurement": "visible_light"},
    "weather/light/infrared": {"measurement": "infrared"},
    "weather/lightning/distance": {"measurement": "lightning_distance"}
}

# Connect to InfluxDB
influx_client = InfluxDBClient(
    host=INFLUXDB_HOST,
    port=INFLUXDB_PORT,
    username=INFLUXDB_USER,
    password=INFLUXDB_PASSWORD,
    database=INFLUXDB_DATABASE
)

def on_connect(client, userdata, flags, rc):
    """Callback for when the client connects to the MQTT broker."""
    connection_responses = {
        0: "Connected successfully",
        1: "Incorrect protocol version",
        2: "Invalid client identifier",
        3: "Server unavailable",
        4: "Bad username or password",
        5: "Not authorized"
    }
    print(f"MQTT Connection result: {connection_responses.get(rc, f'Unknown error ({rc})')}")
    
    if rc == 0:
        print(f"Connected to MQTT broker: {MQTT_BROKER} on port {MQTT_PORT}")
        client.subscribe(WEATHER_TOPIC)
        print(f"Subscribed to topic: {WEATHER_TOPIC}")
    else:
        print("Failed to connect to MQTT broker. Exiting...")
        exit(1)

def on_message(client, userdata, msg):
    """Callback for when a message is received from the MQTT broker."""
    try:
        topic = msg.topic
        payload = msg.payload.decode()
        print(f"Received message on topic {topic}: {payload}")
        
        # Handle the message based on the topic
        if topic in TOPIC_MAPPING:
            mapping = TOPIC_MAPPING[topic]
            measurement = mapping["measurement"]
            # field = mapping["field"]
            
            # For summary topic (JSON data)
            if topic == "weather/summary":
                json_data = json.loads(payload)
                # Create point with all fields from summary
                json_body = [{
                    "measurement": "weather_summary",
                    "time": int(time.time() * 1000000000),  # nanoseconds precision
                    "fields": {
                        "temperatureF": float(json_data["temperatureF"]),
                        "humidity": float(json_data["humidity"]),
                        "pressure": float(json_data["pressure"]),
                        "tvoc": int(json_data["tvoc"]),
                        "eco2": int(json_data["eco2"]),
                        "aqi": int(json_data["aqi"]),
                        "light_level": int(json_data["light_level"]),
                        "lightning_distance": float(json_data["lightning_distance"])
                    }
                }]
            else:
                # Create point with single field for individual topics
                try:
                    # Try to convert to float first
                    value = float(payload)
                except ValueError:
                    # If not a number, keep as string
                    value = payload
                
                json_body = [{
                    "measurement": measurement,
                    # "tags": {
                    #     "sensor": topic.split('/')[1],  # e.g., "ens160", "bme280"
                    #     "parameter": topic.split('/')[-1]  # e.g., "aqi", "humidity"
                    # },
                    "time": int(time.time() * 1000000000),  # nanoseconds precision
                    "fields": {
                        # field: value
                        "value": value
                    }
                }]
            
            # Write to InfluxDB
            success = influx_client.write_points(json_body)
            if success:
                print(f"Data written to InfluxDB: {measurement}")
            else:
                print(f"Failed to write data to InfluxDB: {measurement}")
                
    except json.JSONDecodeError:
        print(f"Error: Cannot parse JSON from topic {topic}: {payload}")
    except Exception as e:
        print(f"Error processing message on topic {topic}: {e}")

def main():
    # Initialize MQTT client
    client = mqtt.Client(client_id=MQTT_CLIENT_ID)
    
    # Set callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Set authentication if needed
    if MQTT_USERNAME and MQTT_PASSWORD:
        client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    try:
        # Check if InfluxDB database exists, create if not
        databases = influx_client.get_list_database()
        if {'name': INFLUXDB_DATABASE} not in databases:
            print(f"Creating database '{INFLUXDB_DATABASE}'")
            influx_client.create_database(INFLUXDB_DATABASE)
        
        influx_client.switch_database(INFLUXDB_DATABASE)
        print(f"Connected to InfluxDB: {INFLUXDB_HOST}:{INFLUXDB_PORT}")
        
        # Connect to MQTT broker
        print(f"Connecting to MQTT broker: {MQTT_BROKER}:{MQTT_PORT}...")
        client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
        
        # Start the loop
        print("Starting Weather MQTT to InfluxDB bridge. Press Ctrl+C to stop.")
        client.loop_forever()
        
    except KeyboardInterrupt:
        print("\nStopping the bridge...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        # Clean up
        client.disconnect()
        print("Bridge stopped.")

if __name__ == "__main__":
    main()
