// publish-test-data.js
require('dotenv').config();
const mqtt = require('mqtt');

const client = mqtt.connect('mqtt://localhost:5173/mqtt', {
  clientId: `weather_wizards_${Math.random().toString(16).substring(2, 10)}`,
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
    
    console.log(`Publishing - Temp: ${temperature}°C, Humidity: ${humidity}%`);
    
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