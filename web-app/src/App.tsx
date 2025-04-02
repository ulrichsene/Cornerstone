import React, { useState, useEffect } from 'react';
import { Thermometer, Droplets, Gauge, Zap, Wind, Sun } from 'lucide-react';
import { WeatherCard } from './components/WeatherCard';
import { WeatherData } from './types';
import { mqttClient } from './mqtt/client';

function getAirQualityDescription(value: number): string {
  if (value <= 50) return 'Good';
  if (value <= 100) return 'Moderate';
  if (value <= 150) return 'Unhealthy for Sensitive Groups';
  if (value <= 200) return 'Unhealthy';
  if (value <= 300) return 'Very Unhealthy';
  return 'Hazardous';
}

function getLightLevelDescription(value: number): string {
  if (value < 50) return 'Dark';
  if (value < 1000) return 'Indoor Light';
  if (value < 10000) return 'Overcast';
  if (value < 30000) return 'Daylight';
  return 'Direct Sunlight';
}

function App() {
  const [weatherData, setWeatherData] = useState<WeatherData>({
    temperature: 23.5,
    humidity: 45,
    pressure: 1013,
    lightning: 0,
    airQuality: 35,
    lightLevel: 15000,
    timestamp: new Date().toISOString()
  });

  useEffect(() => {
    // Connect to MQTT broker
    mqttClient.connect();

    // Set up MQTT message handler
    mqttClient.onUpdate((update) => {
      setWeatherData(prev => ({
        ...prev,
        ...update
      }));
    });

    // Cleanup on unmount
    return () => {
      mqttClient.disconnect();
    };
  }, []);

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-blue-100">
      <div className="container mx-auto px-4 py-8">
        <header className="text-center mb-12">
          <h1 className="text-4xl font-bold text-gray-800 mb-2">Weather Wizards</h1>
          <p className="text-gray-600">
            Last updated: {new Date(weatherData.timestamp).toLocaleTimeString()}
          </p>
        </header>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          <WeatherCard
            title="Temperature"
            value={Number(weatherData.temperature.toFixed(1))}
            unit="°C"
            icon={<Thermometer size={32} />}
          />
          <WeatherCard
            title="Humidity"
            value={Number(weatherData.humidity.toFixed(1))}
            unit="%"
            icon={<Droplets size={32} />}
          />
          <WeatherCard
            title="Air Pressure"
            value={Number(weatherData.pressure.toFixed(0))}
            unit="hPa"
            icon={<Gauge size={32} />}
          />
          <WeatherCard
            title="Lightning Strikes"
            value={weatherData.lightning}
            unit="strikes/min"
            icon={<Zap size={32} />}
          />
          <WeatherCard
            title="Air Quality"
            value={Number(weatherData.airQuality.toFixed(0))}
            unit="AQI"
            icon={<Wind size={32} />}
            description={getAirQualityDescription(weatherData.airQuality)}
          />
          <WeatherCard
            title="Light Level"
            value={Number(weatherData.lightLevel.toFixed(0))}
            unit="lux"
            icon={<Sun size={32} />}
            description={getLightLevelDescription(weatherData.lightLevel)}
          />
        </div>
      </div>
    </div>
  );
}

export default App;