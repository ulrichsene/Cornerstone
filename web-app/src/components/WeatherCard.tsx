import React from 'react';
import { WeatherCardProps } from '../types';

export function WeatherCard({ title, value, unit, icon, description }: WeatherCardProps) {
  return (
    <div className="bg-white rounded-xl shadow-lg p-6 flex flex-col items-center">
      <div className="text-blue-600 mb-4">
        {icon}
      </div>
      <h3 className="text-gray-600 font-medium text-sm mb-2">{title}</h3>
      <div className="flex items-end mb-2">
        <span className="text-3xl font-bold text-gray-800">{value}</span>
        <span className="text-gray-600 ml-1">{unit}</span>
      </div>
      {description && (
        <p className="text-sm text-gray-500 text-center">{description}</p>
      )}
    </div>
  );
}