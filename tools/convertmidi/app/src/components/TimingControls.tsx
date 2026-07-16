import React from 'react';
import { Clock, RotateCcw, Music } from 'lucide-react';
import type { MidiNote } from '../types/midi';

interface TimingControlsProps {
  bpm: number;
  subdivision: number;
  timeOffset: number;
  onBpmChange: (value: number) => void;
  onSubdivisionChange: (value: number) => void;
  onTimeOffsetChange: (value: number) => void;
  originalNotes: MidiNote[];
}

export const TimingControls: React.FC<TimingControlsProps> = ({
  bpm,
  subdivision,
  timeOffset,
  onBpmChange,
  onSubdivisionChange,
  onTimeOffsetChange,
  originalNotes
}) => {
  const minTime = originalNotes.length > 0 ? Math.min(...originalNotes.map(n => n.time)) : 0;
  const maxTime = originalNotes.length > 0 ? Math.max(...originalNotes.map(n => n.time)) : 0;
  const timeRange = maxTime - minTime;

  const handleReset = () => {
    onBpmChange(120);
    onSubdivisionChange(8);
    onTimeOffsetChange(0);
  };

  const subdivisionNames: Record<number, string> = {
    1: 'Whole Notes',
    2: 'Half Notes', 
    4: 'Quarter Notes',
    8: 'Eighth Notes',
    16: 'Sixteenth Notes',
    32: '32nd Notes',
    64: '64th Notes'
  };

  // Calculate musical timing info
  const beatsPerSecond = bpm / 60;
  const subdivisionPerSecond = beatsPerSecond * (subdivision / 4);
  const totalSteps = Math.max(16, Math.ceil(timeRange * subdivisionPerSecond));
  const totalPages = Math.ceil(totalSteps / 16);

  return (
    <div className="bg-gray-800 rounded-xl p-6">
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center space-x-2">
          <Music className="w-5 h-5 text-blue-400" />
          <h3 className="text-lg font-semibold text-white">Musical Timing Controls</h3>
        </div>
        <button
          onClick={handleReset}
          className="flex items-center space-x-1 px-3 py-1 bg-gray-700 text-gray-300 rounded-lg hover:bg-gray-600 transition-colors text-sm"
        >
          <RotateCcw className="w-4 h-4" />
          <span>Reset</span>
        </button>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        {/* BPM */}
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <label className="text-sm font-medium text-gray-300">
              BPM (Beats Per Minute)
            </label>
            <span className="text-sm text-gray-400 bg-gray-700 px-2 py-1 rounded">
              {bpm}
            </span>
          </div>
          <div className="space-y-2">
            <input
              type="range"
              min="60"
              max="200"
              step="1"
              value={bpm}
              onChange={(e) => onBpmChange(parseInt(e.target.value))}
              className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer slider"
            />
            <div className="flex justify-between text-xs text-gray-500">
              <span>60</span>
              <span>130</span>
              <span>200</span>
            </div>
          </div>
          <input
            type="number"
            min="30"
            max="300"
            step="1"
            value={bpm}
            onChange={(e) => onBpmChange(parseInt(e.target.value) || 120)}
            className="w-full px-3 py-2 bg-gray-700 text-white rounded-lg border border-gray-600 focus:border-blue-400 focus:outline-none"
          />
        </div>

        {/* Subdivision */}
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <label className="text-sm font-medium text-gray-300">
              Note Subdivision
            </label>
            <span className="text-sm text-gray-400 bg-gray-700 px-2 py-1 rounded">
              {subdivisionNames[subdivision] || `1/${subdivision}`}
            </span>
          </div>
          <div className="space-y-2">
            <select
              value={subdivision}
              onChange={(e) => onSubdivisionChange(parseInt(e.target.value))}
              className="w-full px-3 py-2 bg-gray-700 text-white rounded-lg border border-gray-600 focus:border-blue-400 focus:outline-none"
            >
              <option value={1}>Whole Notes (1/1)</option>
              <option value={2}>Half Notes (1/2)</option>
              <option value={4}>Quarter Notes (1/4)</option>
              <option value={8}>Eighth Notes (1/8)</option>
              <option value={16}>Sixteenth Notes (1/16)</option>
              <option value={32}>32nd Notes (1/32)</option>
              <option value={64}>64th Notes (1/64)</option>
            </select>
          </div>
          <div className="text-xs text-gray-500">
            Each step = 1/{subdivision} note
          </div>
        </div>

        {/* Time Offset */}
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <label className="text-sm font-medium text-gray-300">
              Time Offset (seconds)
            </label>
            <span className="text-sm text-gray-400 bg-gray-700 px-2 py-1 rounded">
              {timeOffset.toFixed(2)}s
            </span>
          </div>
          <div className="space-y-2">
            <input
              type="range"
              min={-timeRange}
              max={timeRange}
              step="0.01"
              value={timeOffset}
              onChange={(e) => onTimeOffsetChange(parseFloat(e.target.value))}
              className="w-full h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer slider"
            />
            <div className="flex justify-between text-xs text-gray-500">
              <span>{(-timeRange).toFixed(1)}s</span>
              <span>0s</span>
              <span>{timeRange.toFixed(1)}s</span>
            </div>
          </div>
          <input
            type="number"
            step="0.01"
            value={timeOffset}
            onChange={(e) => onTimeOffsetChange(parseFloat(e.target.value) || 0)}
            className="w-full px-3 py-2 bg-gray-700 text-white rounded-lg border border-gray-600 focus:border-blue-400 focus:outline-none"
          />
        </div>
      </div>

      {/* Musical Info */}
      <div className="mt-4 p-3 bg-gray-900 rounded-lg">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 text-sm">
          <div>
            <span className="text-gray-400">Time Range:</span>
            <span className="text-white ml-2">{timeRange.toFixed(2)}s</span>
          </div>
          <div>
            <span className="text-gray-400">Steps/Second:</span>
            <span className="text-white ml-2">{subdivisionPerSecond.toFixed(1)}</span>
          </div>
          <div>
            <span className="text-gray-400">Total Steps:</span>
            <span className="text-white ml-2">{totalSteps}</span>
          </div>
          <div>
            <span className="text-gray-400">Total Pages:</span>
            <span className="text-white ml-2">{totalPages}</span>
          </div>
        </div>
      </div>
    </div>
  );
};