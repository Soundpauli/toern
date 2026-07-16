import React, { useState } from 'react';
import { GripVertical, Plus, AlertTriangle } from 'lucide-react';
import type { TrackInfo } from '../types/midi';

interface TrackLegendProps {
  tracks: TrackInfo[];
  enabledTracks: Set<number>;
  onTrackToggle: (trackId: number, enabled: boolean) => void;
  onTrackReorder?: (newOrder: TrackInfo[]) => void;
  onAddEmptyTrack?: () => void;
}

// TŒRN only supports 8 voices (channels 1-8)
const MAX_TOERN_VOICES = 8;

export const TrackLegend: React.FC<TrackLegendProps> = ({ 
  tracks, 
  enabledTracks, 
  onTrackToggle,
  onTrackReorder,
  onAddEmptyTrack
}) => {
  const [draggedTrack, setDraggedTrack] = useState<number | null>(null);
  const [dragOverIndex, setDragOverIndex] = useState<number | null>(null);

  if (tracks.length === 0) return null;

  const enabledCount = Array.from(enabledTracks).filter(id => id < tracks.length).length;
  const hasExcessTracks = enabledCount > MAX_TOERN_VOICES;

  const handleDragStart = (e: React.DragEvent, trackIndex: number) => {
    setDraggedTrack(trackIndex);
    e.dataTransfer.effectAllowed = 'move';
  };

  const handleDragOver = (e: React.DragEvent, index: number) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
    setDragOverIndex(index);
  };

  const handleDragLeave = () => {
    setDragOverIndex(null);
  };

  const handleDrop = (e: React.DragEvent, dropIndex: number) => {
    e.preventDefault();
    
    if (draggedTrack === null || draggedTrack === dropIndex) {
      setDraggedTrack(null);
      setDragOverIndex(null);
      return;
    }

    const newTracks = [...tracks];
    const draggedItem = newTracks[draggedTrack];
    
    // Remove the dragged item
    newTracks.splice(draggedTrack, 1);
    
    // Insert at new position
    newTracks.splice(dropIndex, 0, draggedItem);
    
    // Update track IDs to match new order
    const reorderedTracks = newTracks.map((track, index) => ({
      ...track,
      id: index
    }));

    if (onTrackReorder) {
      onTrackReorder(reorderedTracks);
    }

    setDraggedTrack(null);
    setDragOverIndex(null);
  };

  const handleDragEnd = () => {
    setDraggedTrack(null);
    setDragOverIndex(null);
  };

  return (
    <div className="bg-gray-800 rounded-xl p-6">
      <div className="flex items-center justify-between mb-4">
        <div>
          <h3 className="text-lg font-semibold text-white">Track Legend</h3>
          <p className="text-xs text-gray-500 mt-1">
            Drag to reorder. First 8 enabled tracks become TŒRN voices 1-8.
          </p>
        </div>
        {onAddEmptyTrack && tracks.length < MAX_TOERN_VOICES && (
          <button
            onClick={onAddEmptyTrack}
            className="flex items-center space-x-1 px-3 py-1 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors text-sm"
          >
            <Plus className="w-4 h-4" />
            <span>Add Empty Track</span>
          </button>
        )}
      </div>
      
      {hasExcessTracks && (
        <div className="flex items-center space-x-2 p-3 mb-4 bg-yellow-900/30 border border-yellow-600/50 rounded-lg">
          <AlertTriangle className="w-5 h-5 text-yellow-500 flex-shrink-0" />
          <p className="text-sm text-yellow-200">
            TŒRN supports max 8 voices. {enabledCount - MAX_TOERN_VOICES} track(s) will be ignored.
            Disable some tracks or reorder to prioritize.
          </p>
        </div>
      )}
      
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-3">
        {tracks.map((track, index) => {
          // Calculate which TŒRN voice this track maps to (if enabled and within first 8)
          const enabledBefore = tracks.slice(0, index).filter(t => enabledTracks.has(t.id)).length;
          const isEnabled = enabledTracks.has(track.id);
          const voiceNumber = isEnabled ? enabledBefore + 1 : null;
          const isOverLimit = voiceNumber !== null && voiceNumber > MAX_TOERN_VOICES;
          
          return (
          <div 
            key={`${track.id}-${index}`}
            draggable
            onDragStart={(e) => handleDragStart(e, index)}
            onDragOver={(e) => handleDragOver(e, index)}
            onDragLeave={handleDragLeave}
            onDrop={(e) => handleDrop(e, index)}
            onDragEnd={handleDragEnd}
            className={`flex items-center space-x-3 p-2 rounded-lg hover:bg-gray-700 transition-colors cursor-move ${
              draggedTrack === index ? 'opacity-50' : ''
            } ${
              dragOverIndex === index ? 'bg-gray-600 border-2 border-blue-400' : ''
            } ${
              isOverLimit ? 'opacity-40' : ''
            }`}
          >
            <GripVertical className="w-4 h-4 text-gray-500 flex-shrink-0" />
            <input
              type="checkbox"
              checked={isEnabled}
              onChange={(e) => onTrackToggle(track.id, e.target.checked)}
              className="w-4 h-4 text-blue-600 bg-gray-700 border-gray-600 rounded focus:ring-blue-500 focus:ring-2 flex-shrink-0"
            />
            <div
              className={`w-4 h-4 rounded-full border border-gray-600 transition-opacity flex-shrink-0 ${
                isEnabled && !isOverLimit ? 'opacity-100' : 'opacity-30'
              }`}
              style={{ backgroundColor: track.color }}
            />
            <div className="flex-1 min-w-0">
              <div className="flex items-center space-x-2">
                {voiceNumber && !isOverLimit && (
                  <span className="text-xs font-mono px-1.5 py-0.5 rounded bg-blue-600 text-white">
                    V{voiceNumber}
                  </span>
                )}
                {isOverLimit && (
                  <span className="text-xs font-mono px-1.5 py-0.5 rounded bg-yellow-600/50 text-yellow-200">
                    skip
                  </span>
                )}
                <p className={`text-sm font-medium truncate transition-opacity ${
                  isEnabled && !isOverLimit ? 'text-white' : 'text-gray-500'
                }`}>
                  {track.name}
                </p>
              </div>
              <p className={`text-xs transition-opacity ${
                isEnabled && !isOverLimit ? 'text-gray-400' : 'text-gray-600'
              }`}>
                {track.noteCount} notes
              </p>
            </div>
          </div>
        );
        })}
      </div>
    </div>
  );
};