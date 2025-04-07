#!/bin/bash

# Load environment variables
if [ -f .env ]; then
  export $(grep -v '^#' .env | xargs)
  echo "✅ Loaded environment variables."
else
  echo "❌ .env file not found!"
  exit 1
fi

echo ""
TOPIC="test/topic"
MESSAGE="Hello, MQTT!"
HOST="localhost"
PORT="5173"
USER="weather_user"

echo "📡 Subscribing to topic: $TOPIC"
mosquitto_sub -h $HOST -p $PORT -t "$TOPIC" -u "$USER" -P "$MQTT_PASSWORD" &
SUB_PID=$!

sleep 1
echo ""
echo "📤 Publishing test message: \"$MESSAGE\" to topic: $TOPIC"
mosquitto_pub -h $HOST -p $PORT -t "$TOPIC" -m "$MESSAGE" -u "$USER" -P "$MQTT_PASSWORD"

sleep 3
kill $SUB_PID 2>/dev/null

echo ""
echo "✅ Test complete."
