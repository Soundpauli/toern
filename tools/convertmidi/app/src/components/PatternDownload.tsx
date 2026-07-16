import React from 'react';
import { Download, FileText } from 'lucide-react';
import type { GridNote } from '../types/midi';

interface PatternDownloadProps {
  notes: GridNote[];
  currentPage: number;
  exportStartPage: number;
  onExportStartPageChange: (page: number) => void;
  totalPages: number;
  fileName: string;
}

export const PatternDownload: React.FC<PatternDownloadProps> = ({ 
  notes, 
  currentPage, 
  exportStartPage,
  onExportStartPageChange,
  totalPages,
  fileName 
}) => {
  const generateExportPattern = (): Uint8Array => {
    const maxExportPages = 16;
    const actualExportPages = Math.min(maxExportPages, totalPages - exportStartPage);

    // 256 Steps (X), 16 Rows (Y, 1-basiert wie Firmware)
    const continuousGrid: Array<Array<{ channel: number; velocity: number } | null>> =
      Array(256).fill(null).map(() => Array(16).fill(null));

    // Fill grid: notes mit note.y von 0..14 auf Y=0..14, Y=15 bleibt leer
    notes.forEach(note => {
      if (note.page >= exportStartPage && note.page < exportStartPage + maxExportPages) {
        const continuousX = (note.page - exportStartPage) * 16 + note.x;
        if (continuousX < 256 && note.y >= 0 && note.y < 15) {
          if (continuousGrid[continuousX][note.y] === null) {
            continuousGrid[continuousX][note.y] = {
              channel: note.track + 1,
              velocity: Math.round(note.velocity * 127)
            };
          }
        }
      }
    });

    // Firmware erwartet: für x=1..256, y=1..16 (1-basiert)
    // Jede Zelle: 4 Bytes (channel, velocity, probability=100, condition=1)
    const allData: number[] = [];
    for (let x = 0; x < 256; x++) {
      for (let y = 0; y < 16; y++) {
        const cell = continuousGrid[x][y];
        if (cell) {
          allData.push(cell.channel);      // channel
          allData.push(cell.velocity);     // velocity
          allData.push(100);               // probability default
          allData.push(1);                 // condition default
        } else {
          allData.push(0);                 // channel
          allData.push(0);                 // velocity
          allData.push(100);               // probability default
          allData.push(1);                 // condition default
        }
      }
    }

    return new Uint8Array(allData);
  };

  const handleDownloadExportPages = () => {
    const patternData = generateExportPattern();
    const blob = new Blob([patternData], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    const maxExportPages = 16;
    const actualExportPages = Math.min(maxExportPages, totalPages - exportStartPage);
    a.download = `${fileName.replace('.mid', '')}_pages_${exportStartPage + 1}-${exportStartPage + actualExportPages}.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  const maxExportPages = 16;
  const actualExportPages = Math.min(maxExportPages, totalPages - exportStartPage);

  return (
    <div className="bg-gray-800 rounded-xl p-6">
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center space-x-2">
          <FileText className="w-5 h-5 text-blue-400" />
          <h3 className="text-lg font-semibold text-white">Pattern Export</h3>
        </div>
      </div>
      
      <div className="space-y-4">
        {/* Export Start Page Selector */}
        <div className="space-y-2">
          <label className="text-sm font-medium text-gray-300">
            Export Start Page (max 16 pages from this point)
          </label>
          <div className="flex items-center space-x-3">
            <input
              type="range"
              min="0"
              max={Math.max(0, totalPages - 1)}
              value={exportStartPage}
              onChange={(e) => onExportStartPageChange(parseInt(e.target.value))}
              className="flex-1 h-2 bg-gray-700 rounded-lg appearance-none cursor-pointer slider"
            />
            <span className="text-sm text-gray-400 bg-gray-700 px-2 py-1 rounded min-w-[60px] text-center">
              {exportStartPage + 1}
            </span>
          </div>
          <div className="text-xs text-gray-500">
            Will export pages {exportStartPage + 1} to {exportStartPage + actualExportPages} ({actualExportPages} pages total)
          </div>
        </div>
        
        <div className="text-sm text-gray-400">
          Export pattern data in the same binary format as your C savePattern() function.
          Each grid position contains: channel (track + 1) and velocity (0-127).
        </div>
        
        <div className="flex flex-col sm:flex-row gap-3">
          <button
            onClick={handleDownloadExportPages}
            className="flex items-center justify-center space-x-2 px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 transition-colors"
          >
            <Download className="w-4 h-4" />
            <span>Download Pages {exportStartPage + 1}-{exportStartPage + actualExportPages}</span>
          </button>
        </div>
        
        <div className="text-xs text-gray-500 bg-gray-900 p-3 rounded-lg">
          <div className="font-semibold mb-2">Binary Format:</div>
          <div>• Each position: 2 bytes (channel, velocity)</div>
          <div>• Grid size: 256x15 = 3840 positions = 7680 bytes per export</div>
          <div>• Export limit: 16 pages maximum (256 x 15 grid)</div>
          <div>• Channel: 0 = empty, 1-8 = voice number</div>
          <div>• Velocity: 0-127 (MIDI standard)</div>
          <div>• Rows 1-15 (15 pitches per voice, row 16 reserved)</div>
        </div>
      </div>
    </div>
  );
};