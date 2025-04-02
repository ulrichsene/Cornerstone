import mqtt from 'mqtt';
import { WeatherData } from '../types';

const MQTT_BROKER_URL = 'wss://your-mqtt-broker:8084/mqtt'; // Updated to use secure WebSocket
const MQTT_TOPICS = {
  temperature: 'weather/temperature',
  humidity: 'weather/humidity',
  pressure: 'weather/pressure',
  lightning: 'weather/lightning',
  airQuality: 'weather/air-quality',
  lightLevel: 'weather/light-level'
};

class MQTTClient {
  private client: mqtt.MqttClient | null = null;
  private onUpdateCallback: ((data: Partial<WeatherData>) => void) | null = null;

  connect() {
    try {
      this.client = mqtt.connect(MQTT_BROKER_URL, {
        clientId: `weather_wizards_${Math.random().toString(16).substring(2, 10)}`,
        clean: true,
        connectTimeout: 4000,
        reconnectPeriod: 1000,
      });

      this.client.on('connect', () => {
        console.log('Connected to MQTT broker');
        this.subscribeToTopics();
      });

      this.client.on('message', (topic: string, message: Buffer) => {
        this.handleMessage(topic, message);
      });

      this.client.on('error', (error) => {
        console.error('MQTT connection error:', error);
      });
    } catch (error) {
      console.error('Failed to connect to MQTT broker:', error);
    }
  }

  private subscribeToTopics() {
    if (!this.client) return;
    
    Object.values(MQTT_TOPICS).forEach(topic => {
      this.client?.subscribe(topic, (err) => {
        if (err) {
          console.error(`Failed to subscribe to ${topic}:`, err);
        } else {
          console.log(`Subscribed to ${topic}`);
        }
      });
    });
  }

  private handleMessage(topic: string, message: Buffer) {
    if (!this.onUpdateCallback) return;

    try {
      const value = parseFloat(message.toString());
      if (isNaN(value)) return;

      const update: Partial<WeatherData> = {
        timestamp: new Date().toISOString()
      };

      switch (topic) {
        case MQTT_TOPICS.temperature:
          update.temperature = value;
          break;
        case MQTT_TOPICS.humidity:
          update.humidity = value;
          break;
        case MQTT_TOPICS.pressure:
          update.pressure = value;
          break;
        case MQTT_TOPICS.lightning:
          update.lightning = value;
          break;
        case MQTT_TOPICS.airQuality:
          update.airQuality = value;
          break;
        case MQTT_TOPICS.lightLevel:
          update.lightLevel = value;
          break;
      }

      this.onUpdateCallback(update);
    } catch (error) {
      console.error('Error handling MQTT message:', error);
    }
  }

  onUpdate(callback: (data: Partial<WeatherData>) => void) {
    this.onUpdateCallback = callback;
  }

  disconnect() {
    if (this.client) {
      this.client.end();
      this.client = null;
    }
  }
}

export const mqttClient = new MQTTClient();