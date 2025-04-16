import { useEffect, useState } from 'react';
import mqtt from 'mqtt';
import { Thermometer, Wind, Sun, Cloud, Zap, RotateCw } from 'lucide-react';
import { SensorCard } from './components/SensorCard';
import type { SensorMessage, ComboData, BME280Data, BME680Data, LightData, LightningData, SummaryData } from './types';

function App() {
  const [connectionStatus, setConnectionStatus] = useState<'connecting' | 'connected' | 'disconnected'>('disconnected');
  const [ens160Data, setEns160Data] = useState<ComboData | null>(null);
  const [bme280Data, setBme280Data] = useState<BME280Data | null>(null);
  const [bme680Data, setBme680Data] = useState<BME680Data | null>(null);
  const [lightData, setLightData] = useState<LightData | null>(null);
  const [lightningData, setLightningData] = useState<LightningData | null>(null);
  const [summaryData, setSummaryData] = useState<SummaryData | null>(null);
  const [lastLightningTime, setLastLightningTime] = useState<Date | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [currentTime, setCurrentTime] = useState(new Date());
  const [client, setClient] = useState<mqtt.MqttClient | null>(null);

  const connectMQTT = () => {
    if (client) {
      client.end();
    }

    setConnectionStatus('connecting');
    const newClient = mqtt.connect('ws://localhost:9001', {
      username: 'weather_user',
      password: 'WeatherWizards'
    });

    const topics = [
      'weather/ens160/+',
      'weather/bme280/+',
      'weather/bme680/+',
      'weather/light/+',
      'weather/lightning/+',
      'weather/summary'
    ];

    newClient.on('connect', () => {
      console.log('✅ MQTT connected via WebSocket');
      setConnectionStatus('connected');
      setError(null);
    
      topics.forEach(topic => {
        newClient.subscribe(topic, (err) => {
          if (err) {
            console.error("❌ Failed to subscribe to", topic, err);
          } else {
            console.log("📬 Subscribed to", topic);
          }
        });
      });
    });
    

    newClient.on('error', (err) => {
      console.error('MQTT Error:', err);
      setError(`Connection error: ${err.message}`);
      setConnectionStatus('disconnected');
    });

    newClient.on('message', (topic, message) => {
      try {
        const value = message.toString();
        console.log("📡 MQTT Message:", topic, value); // 👈 Add this line
    
        if (topic === 'weather/summary') {
          setSummaryData(JSON.parse(value));
          return;
        }
    
        const [___, sensor, measurement] = topic.split('/');
        
        switch (sensor) {
          case 'ens160':
            setEns160Data(prev => ({
              ...prev,
              [measurement]: Number(value)
            } as ComboData));
            break;
          case 'bme280':
            setBme280Data(prev => ({
              ...prev,
              [measurement]: Number(value)
            } as BME280Data));
            break;
          case 'bme680':
            const updatedBme680 = {
              ...bme680Data,
              [measurement]: Number(value)
            };
            console.log("🌡️ Updated bme680Data:", updatedBme680);
            setBme680Data(updatedBme680 as BME680Data);
            break;  
          case 'light':
            setLightData(prev => ({
              ...prev,
              [measurement]: Number(value)
            } as LightData));
            break;
          case 'lightning':
            if (measurement === 'strike' && value === '1') {
              setLastLightningTime(new Date());
            }
            setLightningData(prev => ({
              ...prev,
              [measurement]: measurement === 'strike' ? value : Number(value)
            } as LightningData));
            break;
        }
      } catch (err) {
        console.error('Error processing message:', err);
        //setError(`Error processing data: ${err.message}`);
      }
    });

    setClient(newClient);
    return newClient;
  };

  useEffect(() => {
    const newClient = connectMQTT();
    
    // Update time every second
    const timeInterval = setInterval(() => {
      setCurrentTime(new Date());
    }, 1000);

    return () => {
      if (newClient) {
        newClient.end();
      }
      clearInterval(timeInterval);
    };
  }, []);

  const handleReload = () => {
    connectMQTT();
  };

  return (
    <div className="min-h-screen gradient-bg p-8">
      <div className="max-w-7xl mx-auto">
        <div className="flex items-center justify-between mb-12">
          <div className="flex items-center gap-3">
            <Cloud className="w-10 h-10 text-blue-400" />
            <h1 className="text-4xl font-bold text-white">Weather Station Dashboard</h1>
          </div>
          <div className="flex items-center gap-6">
            <div className="text-white text-right">
              <div className="text-2xl font-bold">
                {currentTime.toLocaleTimeString()}
              </div>
              <div className="text-sm opacity-80">
                {currentTime.toLocaleDateString(undefined, { 
                  weekday: 'long', 
                  year: 'numeric', 
                  month: 'long', 
                  day: 'numeric' 
                })}
              </div>
            </div>
            <button 
              onClick={handleReload}
              className="p-2 rounded-full hover:bg-white/10 transition-colors duration-200"
              title="Reload Connection"
            >
              <RotateCw className="w-6 h-6 text-white" />
            </button>
            <div className="flex items-center gap-2">
              <div className={`w-3 h-3 rounded-full ${
                connectionStatus === 'connected' ? 'bg-green-500' :
                connectionStatus === 'connecting' ? 'bg-yellow-500' :
                'bg-red-500'
              }`} />
              <span className="text-white capitalize">{connectionStatus}</span>
            </div>
          </div>
        </div>

        {error && (
          <div className="mb-6 p-4 bg-red-100 border border-red-200 rounded-lg text-red-700">
            {error}
          </div>
        )}
        
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
          <SensorCard title="Environmental" icon={<Wind className="w-6 h-6 text-blue-600" />}>
            <div className="space-y-4">
              <div>
                <div className="sensor-label">Air Quality Index</div>
                <div className="sensor-value">{ens160Data?.aqi || 'N/A'}</div>
              </div>
              <div>
                <div className="sensor-label">TVOC</div>
                <div className="sensor-value">{ens160Data?.tvoc || 'N/A'} <span className="text-sm text-gray-500">ppb</span></div>
              </div>
              <div>
                <div className="sensor-label">CO₂</div>
                <div className="sensor-value">{ens160Data?.eco2 || 'N/A'} <span className="text-sm text-gray-500">ppm</span></div>
              </div>
            </div>
          </SensorCard>

          <SensorCard title="BME680" icon={<Thermometer className="w-6 h-6 text-red-600" />}>
            <div className="grid grid-cols-2 gap-4">
              <div>
                <div className="sensor-label">Temperature</div>
                <div className="sensor-value">{bme680Data?.temperature.toFixed(1) || 'N/A'}°C</div>
              </div>
              <div>
                <div className="sensor-label">Humidity</div>
                <div className="sensor-value">{bme680Data?.humidity.toFixed(1) || 'N/A'}%</div>
              </div>
              <div>
                <div className="sensor-label">Pressure</div>
                <div className="sensor-value">{bme680Data?.pressure.toFixed(0) || 'N/A'} <span className="text-sm text-gray-500">hPa</span></div>
              </div>
              <div>
                <div className="sensor-label">Dew Point</div>
                <div className="sensor-value">{bme680Data?.dew_point_c.toFixed(1) || 'N/A'}°C</div>
              </div>
              <div>
                <div className="sensor-label">Gas</div>
                <div className="sensor-value">{bme680Data?.gas.toFixed(1) || 'N/A'} <span className="text-sm text-gray-500">KΩ</span></div>
              </div>
              <div>
                <div className="sensor-label">Altitude</div>
                <div className="sensor-value">{bme680Data?.altitude.toFixed(0) || 'N/A'} <span className="text-sm text-gray-500">m</span></div>
              </div>
            </div>
          </SensorCard>

          <SensorCard title="Light Levels" icon={<Sun className="w-6 h-6 text-yellow-600" />}>
            <div className="space-y-4">
              <div>
                <div className="sensor-label">Visible + IR</div>
                <div className="sensor-value">{lightData?.visible_ir || 'N/A'}</div>
              </div>
              <div>
                <div className="sensor-label">Infrared</div>
                <div className="sensor-value">{lightData?.infrared || 'N/A'}</div>
              </div>
            </div>
          </SensorCard>

          <SensorCard title="Lightning" icon={<Zap className="w-6 h-6 text-yellow-500" />}>
            <div className="space-y-4">
              <div>
                <div className="sensor-label">Distance</div>
                <div className="sensor-value">
                  {lightningData?.distance 
                    ? `${lightningData.distance} km`
                    : 'No detection'}
                </div>
              </div>
              {lastLightningTime && (
                <div>
                  <div className="sensor-label">Last Strike</div>
                  <div className="text-lg font-semibold text-gray-700">
                    {lastLightningTime.toLocaleTimeString()}
                  </div>
                </div>
              )}
            </div>
          </SensorCard>
        </div>
      </div>
    </div>
  );
}

export default App;