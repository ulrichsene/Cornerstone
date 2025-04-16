export interface ComboData {
  aqi: number;
  tvoc: number;
  eco2: number;
  temperatureF: number;
}

export interface BME280Data {
  humidity: number;
  pressure: number;
  altitude: number;
}

export interface BME680Data {
  temperature: number;
  pressure: number;
  humidity: number;
  dew_point_c: number;
  gas: number;
  altitude: number;
}

export interface LightData {
  visible_ir: number;
  infrared: number;
}

export interface LightningData {
  distance: number;
  strike: string;
}

export interface SummaryData {
  temperatureF: number;
  humidity: number;
  pressure: number;
  tvoc: number;
  eco2: number;
  aqi: number;
  light_level: number;
  lightning_distance: number;
}

export interface SensorMessage {
  type: string;
  data: ComboData | BME280Data | BME680Data | LightData | LightningData | SummaryData;
}