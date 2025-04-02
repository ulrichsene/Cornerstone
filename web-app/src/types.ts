export interface WeatherData {
  temperature: number;
  humidity: number;
  pressure: number;
  lightning: number;
  airQuality: number;
  lightLevel: number;
  timestamp: string;
}

export interface WeatherCardProps {
  title: string;
  value: number;
  unit: string;
  icon: React.ReactNode;
  description?: string;
}