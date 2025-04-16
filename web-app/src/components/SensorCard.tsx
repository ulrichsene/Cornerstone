import React from 'react';

interface SensorCardProps {
  title: string;
  icon: React.ReactNode;
  children: React.ReactNode;
}

export const SensorCard: React.FC<SensorCardProps> = ({ title, icon, children }) => {
  return (
    <div className="card-gradient rounded-xl shadow-lg p-6 border border-gray-100 transition-transform duration-200 hover:scale-[1.02]">
      <div className="flex items-center gap-3 mb-4">
        <div className="p-2 rounded-lg bg-blue-50">
          {icon}
        </div>
        <h2 className="text-lg font-semibold text-gray-800">{title}</h2>
      </div>
      <div className="space-y-4">
        {children}
      </div>
    </div>
  );
};