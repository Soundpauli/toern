import React from 'react';
import { Download, Play, Pause, Square } from 'lucide-react';
import type { GridNote, TrackInfo } from '../types/midi';
import * as Tone from 'tone';

interface NoteGridProps {
  notes: GridNote[];
  tracks: TrackInfo[];
  currentPage: number;
  totalPages: number;
  onPageChange: (page: number) => void;
  onDownloadPattern?: (pageData: Uint8Array) => void;
  bpm: number;
  subdivision: number;
  enabledTracks: Set<number>;
  filterOverlapping: boolean;
  onFilterOverlappingChange: (value: boolean) => void;
  transposeOutOfRange: boolean;
  notePriority: 'highest' | 'lowest';
  onNotePriorityChange: (value: 'highest' | 'lowest') => void;
  pageNoteCount?: (filtered: number, total: number) => void;
}

export const NoteGrid: React.FC<NoteGridProps> = ({ notes, tracks, currentPage, totalPages, onPageChange, onDownloadPattern, bpm, subdivision, enabledTracks, filterOverlapping, onFilterOverlappingChange, transposeOutOfRange, notePriority, onNotePriorityChange, pageNoteCount }) => {
  const [isPlaying, setIsPlaying] = React.useState(false);
  const [currentStep, setCurrentStep] = React.useState(-1);
  const [playingNotes, setPlayingNotes] = React.useState<Set<string>>(new Set());
  const intervalRef = React.useRef<NodeJS.Timeout | null>(null);
  const synthsRef = React.useRef<Map<number, Tone.Synth>>(new Map());
  const limiterRef = React.useRef<Tone.Limiter | null>(null);

  const pageNotes = notes.filter(note => note.page === currentPage);
  
  // Filter overlapping notes:
  // 1) Within same track+step: keep one note per (x, track) based on priority
  // 2) Cross-track at same cell (x, y): keep only one note (highest velocity wins)
  const getFilteredNotes = (notesToFilter: GridNote[]) => {
    // Filter: Nur Noten mit Pitch (note != null/undefined)
    let result = notesToFilter.filter(note => note.note !== null && note.note !== undefined);

    if (filterOverlapping) {
      // Step 1: Per track+step, keep only one note
      const stepTrackMap = new Map<string, GridNote>();
      result.forEach(note => {
        const key = `${note.x}-${note.track}`;
        const existing = stepTrackMap.get(key);
        if (!existing) {
          stepTrackMap.set(key, note);
        } else {
          if (notePriority === 'highest') {
            if (note.y > existing.y) stepTrackMap.set(key, note);
          } else {
            if (note.y < existing.y) stepTrackMap.set(key, note);
          }
        }
      });
      result = Array.from(stepTrackMap.values());
    }

    // Step 2: Cross-track conflicts - one note per (x, y) cell
    // Keep the note with the highest velocity (matches hardware: first-write wins on export)
    const cellMap = new Map<string, GridNote>();
    result.forEach(note => {
      const cellKey = `${note.x}-${note.y}`;
      const existing = cellMap.get(cellKey);
      if (!existing || note.velocity > existing.velocity) {
        cellMap.set(cellKey, note);
      }
    });
    return Array.from(cellMap.values());
  };
  
  const filteredPageNotes = getFilteredNotes(pageNotes);
  const trackColors = tracks.reduce((acc, track) => {
    acc[track.id] = track.color;
    return acc;
  }, {} as Record<number, string>);

  // Initialize limiter + synths for each track
  React.useEffect(() => {
    if (!limiterRef.current) {
      limiterRef.current = new Tone.Limiter(-6).toDestination();
    }
    const limiter = limiterRef.current;

    tracks.filter(track => enabledTracks.has(track.id)).forEach(track => {
      if (!synthsRef.current.has(track.id)) {
        const synth = new Tone.Synth({
          oscillator: {
            type: 'triangle'
          },
          envelope: {
            attack: 0.01,
            decay: 0.1,
            sustain: 0.3,
            release: 0.2
          },
          volume: -12 // reduce per-voice volume to prevent clipping
        }).connect(limiter);
        synthsRef.current.set(track.id, synth);
      }
    });

    return () => {
      synthsRef.current.forEach(synth => synth.dispose());
      synthsRef.current.clear();
      if (limiterRef.current) {
        limiterRef.current.dispose();
        limiterRef.current = null;
      }
    };
  }, [tracks, enabledTracks]);

  // Report page note counts to parent
  React.useEffect(() => {
    if (pageNoteCount) {
      pageNoteCount(filteredPageNotes.length, pageNotes.length);
    }
  }, [filteredPageNotes.length, pageNotes.length, pageNoteCount]);

  const getMidiNoteName = (midiNumber: number): string => {
    const notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
    const octave = Math.floor(midiNumber / 12) - 1;
    const noteName = notes[midiNumber % 12];
    return `${noteName}${octave}`;
  };

  const getGridMidiNote = (note: GridNote): number => {
    const channel = note.track + 1;
    const row = note.y + 1;
    return 47 - channel + row + 12; // +12 = 1 octave up for clearer playback
  };

  const getPlaybackMidiNote = (note: GridNote): number => {
    if (!transposeOutOfRange) {
      return note.note;
    }
    return getGridMidiNote(note);
  };

  const playStep = (step: number) => {
    // Create a map to track one note per track per step
    const trackMap = new Map<number, GridNote>();
    
    // Filter notes for this step and ensure only the highest row note per track
    filteredPageNotes
      .filter(note => note.x === step)
      .forEach(note => {
        // Keep the note with the highest Y position (highest row / highest pitch) for each track at this step
        const existingNote = trackMap.get(note.track);
        if (!existingNote || note.y > existingNote.y) { // Higher Y = higher row
          trackMap.set(note.track, note);
        }
      });
    
    const notesToPlay = Array.from(trackMap.values());
    const playingNoteIds = new Set<string>();

    notesToPlay.forEach(note => {
      const synth = synthsRef.current.get(note.track);
      if (synth) {
        const midiNote = getPlaybackMidiNote(note);
        const frequency = Tone.Frequency(midiNote, 'midi').toFrequency();
        // Use subdivision-based note length
        const noteLength = `${subdivision}n`;
        synth.triggerAttackRelease(frequency, noteLength, undefined, note.velocity);
        playingNoteIds.add(`${note.x}-${note.y + 1}`);
      }
    });

    setPlayingNotes(playingNoteIds);
    
    // Clear playing notes after a short delay
    setTimeout(() => {
      setPlayingNotes(new Set());
    }, 150);
  };

  // Calculate step duration based on BPM and subdivision
  const calculateStepDuration = () => {
    const beatsPerSecond = bpm / 60;
    const subdivisionPerSecond = beatsPerSecond * (subdivision / 4);
    return 1000 / subdivisionPerSecond; // Convert to milliseconds
  };

  const startPlayback = async () => {
    if (Tone.context.state !== 'running') {
      await Tone.start();
    }

    setIsPlaying(true);
    let step = 0;
    let currentPlaybackPage = currentPage;
    setCurrentStep(step);
    
    const stepDuration = calculateStepDuration();
    
    const playCurrentStep = () => {
      // Get notes for the current playback page
      const currentPageNotes = getFilteredNotes(notes.filter(note => note.page === currentPlaybackPage));
      
      // Create a map to track one note per track per step
      const trackMap = new Map<number, GridNote>();
      
      // Filter notes for this step and ensure only the highest row note per track
      currentPageNotes
        .filter(note => note.x === step)
        .forEach(note => {
          // Keep the note with the highest Y value (highest row / highest pitch) for each track at this step
          const existingNote = trackMap.get(note.track);
          if (!existingNote || note.y > existingNote.y) { // Higher Y = higher row
            trackMap.set(note.track, note);
          }
        });
      
      const notesToPlay = Array.from(trackMap.values());
      const playingNoteIds = new Set<string>();

      notesToPlay.forEach(note => {
        const synth = synthsRef.current.get(note.track);
        if (synth) {
          const midiNote = getPlaybackMidiNote(note);
          const frequency = Tone.Frequency(midiNote, 'midi').toFrequency();
          // Use subdivision-based note length
          const noteLength = `${subdivision}n`;
          synth.triggerAttackRelease(frequency, noteLength, undefined, note.velocity);
          playingNoteIds.add(`${note.x}-${note.y + 1}`);
        }
      });

      setPlayingNotes(playingNoteIds);
      
      // Clear playing notes after a short delay
      setTimeout(() => {
        setPlayingNotes(new Set());
      }, 150);
    };
    
    intervalRef.current = setInterval(() => {
      playCurrentStep();
      setCurrentStep(step);
      
      step++;
      if (step >= 16) {
        step = 0;
        // Auto advance to next page or loop back to page 0
        const nextPage = currentPlaybackPage + 1;
        if (nextPage >= totalPages) {
          currentPlaybackPage = 0; // Loop back to page 0
          onPageChange(0); // Update visual page
        } else {
          currentPlaybackPage = nextPage; // Advance to next page
          onPageChange(nextPage); // Update visual page
        }
      }
    }, stepDuration);
  };

  const stopPlayback = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    setIsPlaying(false);
    setCurrentStep(-1);
    setPlayingNotes(new Set());
  };

  const pausePlayback = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    setIsPlaying(false);
  };

  // Cleanup on unmount
  React.useEffect(() => {
    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
      }
    };
  }, []);

  // Generate pattern data for current page in the format: note[x][y].channel, note[x][y].velocity
  const generatePatternData = (): Uint8Array => {
    // Create a 256x15 grid for continuous X positions (TŒRN uses rows 1-15)
    const grid: Array<Array<{ channel: number; velocity: number } | null>> = 
      Array(256).fill(null).map(() => Array(15).fill(null));
    
    // Fill grid with notes from current page, mapping to continuous X positions
    filteredPageNotes.forEach(note => {
      // Nur Noten mit Pitch (note != null/undefined) exportieren
      if (note.note === null || note.note === undefined) return;
      // Calculate continuous X position: page * 16 + x within page
      const continuousX = currentPage * 16 + note.x;
      // note.y is 0-indexed (0-14), maps directly to TŒRN rows 1-15
      // Only use notes that fit in our 256x15 grid and have valid Y range
      if (continuousX < 256 && note.y >= 0 && note.y < 15) {
        if (grid[continuousX][note.y] === null) {
          grid[continuousX][note.y] = {
            channel: note.track + 1, // Track numbers start from 1 (0 means empty)
            velocity: Math.round(note.velocity * 127) // Convert 0-1 to 0-127
          };
        }
      }
    });
    
    // Generate binary data for the full 256x15 grid
    const data: number[] = [];
    
    for (let x = 0; x < 256; x++) {
      for (let y = 0; y < 15; y++) {
        const cell = grid[x][y];
        if (cell) {
          data.push(cell.channel);   // note[x][y].channel
          data.push(cell.velocity);  // note[x][y].velocity
        } else {
          data.push(0);  // Empty channel
          data.push(0);  // Empty velocity
        }
      }
    }
    
    return new Uint8Array(data);
  };

  const handleDownloadPattern = () => {
    if (onDownloadPattern) {
      const patternData = generatePatternData();
      onDownloadPattern(patternData);
    }
  };


  return (
    <div className="bg-gray-800 rounded-xl p-6">
      {/* Playback Controls */}
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-white">
          Page {currentPage + 1} Grid
        </h3>
        <div className="flex items-center space-x-2">
          {!isPlaying ? (
            <button
              onClick={startPlayback}
              className="flex items-center space-x-1 px-3 py-1 bg-green-600 text-white rounded-lg hover:bg-green-700 transition-colors text-sm"
            >
              <Play className="w-4 h-4" />
              <span>Play</span>
            </button>
          ) : (
            <>
              <button
                onClick={pausePlayback}
                className="flex items-center space-x-1 px-3 py-1 bg-yellow-600 text-white rounded-lg hover:bg-yellow-700 transition-colors text-sm"
              >
                <Pause className="w-4 h-4" />
                <span>Pause</span>
              </button>
              <button
                onClick={stopPlayback}
                className="flex items-center space-x-1 px-3 py-1 bg-red-600 text-white rounded-lg hover:bg-red-700 transition-colors text-sm"
              >
                <Square className="w-4 h-4" />
                <span>Stop</span>
              </button>
            </>
          )}
        </div>
      </div>
      
      {/* Grid styled like color_scheme_generator Combined Preview */}
      <div className="flex" style={{ width: '100%' }}>
        {/* Row labels - uses same grid layout as LED matrix for perfect alignment */}
        <div style={{
          display: 'grid',
          gridTemplateRows: 'repeat(15, 1fr)',
          gap: '4px',
          padding: '10px 6px 10px 0', /* matches matrix wrapper padding (6px) + inner padding (4px) */
        }}>
          {Array.from({ length: 15 }, (_, rowIndex) => {
            const displayRow = 15 - rowIndex;
            return (
              <div
                key={displayRow}
                className="relative group"
                style={{
                  aspectRatio: '1',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'flex-end',
                  fontSize: '9px',
                  color: '#666',
                  fontFamily: 'monospace',
                  minWidth: '20px',
                  cursor: 'default',
                }}
              >
                {displayRow}
                {/* Tooltip */}
                <div className="absolute right-full mr-2 top-1/2 transform -translate-y-1/2 opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none z-20">
                  <div style={{ background: '#000', color: '#fff', fontSize: '10px', borderRadius: '4px', padding: '4px 6px', whiteSpace: 'nowrap' }}>
                    <div style={{ fontWeight: 600, marginBottom: '2px' }}>Row {displayRow}:</div>
                    {tracks.filter(track => enabledTracks.has(track.id)).slice(0, 8).map(track => {
                      const channel = track.id + 1;
                      const midiNote = 47 - channel + displayRow;
                      return (
                        <div key={track.id} style={{ display: 'flex', alignItems: 'center', gap: '4px', marginBottom: '1px' }}>
                          <div style={{ width: '8px', height: '8px', borderRadius: '50%', backgroundColor: track.color }} />
                          <span style={{ color: '#aaa' }}>V{channel}:</span>
                          <span>{getMidiNoteName(midiNote)}</span>
                        </div>
                      );
                    })}
                  </div>
                </div>
              </div>
            );
          })}
        </div>

        {/* LED Matrix - exact color_scheme_generator style */}
        <div style={{
          background: '#0a0a0a',
          padding: '6px',
          boxShadow: '0 4px 16px rgba(0,0,0,0.4), inset 0 0 60px rgba(0,0,0,0.6)',
          borderRadius: '4px',
          flex: 1,
        }}>
          <div style={{
            display: 'grid',
            gridTemplateColumns: 'repeat(16, 1fr)',
            gap: '4px',
            background: '#000',
            padding: '4px',
          }}>
            {Array.from({ length: 240 }, (_, index) => {
              const x = index % 16;
              const gridY = Math.floor(index / 16);
              const y = 14 - gridY;

              const notesAtPosition = filteredPageNotes.filter(note => note.x === x && note.y === y);
              const isCurrentlyPlaying = playingNotes.has(`${x}-${y + 1}`);
              const isCurrentStep = currentStep === x;

              if (notesAtPosition.length === 0) {
                // Empty LED - visible grey circle, same size as active notes
                return (
                  <div
                    key={index}
                    style={{
                      width: '100%',
                      aspectRatio: '1',
                      borderRadius: '50%',
                      background: isCurrentStep ? '#444' : '#2a2a2a',
                      boxShadow: isCurrentStep ? '0 0 4px rgba(50,100,255,0.15)' : 'none',
                      position: 'relative',
                    }}
                  />
                );
              }

              const primaryNote = notesAtPosition.reduce((prev, current) =>
                current.velocity > prev.velocity ? current : prev
              );

              const color = trackColors[primaryNote.track] || '#666';
              const vel = isCurrentlyPlaying ? 1 : Math.max(0.3, primaryNote.velocity);

              // Parse color for glow
              const colorMatch = color.match(/\d+/g);
              const cr = colorMatch ? parseInt(colorMatch[0]) : 100;
              const cg = colorMatch ? parseInt(colorMatch[1]) : 100;
              const cb = colorMatch ? parseInt(colorMatch[2]) : 100;
              const glowAlpha = vel * 0.7;
              const glowSpread = isCurrentlyPlaying ? 10 : 5 + vel * 4;

              return (
                <div
                  key={index}
                  className="group"
                  style={{
                    width: '100%',
                    aspectRatio: '1',
                    borderRadius: '50%',
                    background: `rgba(${cr}, ${cg}, ${cb}, ${vel})`,
                    boxShadow: `0 0 ${glowSpread}px rgba(${cr}, ${cg}, ${cb}, ${glowAlpha})`,
                    position: 'relative',
                    cursor: 'pointer',
                    transition: 'transform 0.15s',
                  }}
                  title={`V${primaryNote.track + 1}: ${getMidiNoteName(getPlaybackMidiNote(primaryNote))} (Vel: ${Math.round(primaryNote.velocity * 127)})`}
                  onMouseEnter={(e) => { (e.currentTarget as HTMLElement).style.transform = 'scale(1.3)'; (e.currentTarget as HTMLElement).style.zIndex = '100'; }}
                  onMouseLeave={(e) => { (e.currentTarget as HTMLElement).style.transform = 'scale(1)'; (e.currentTarget as HTMLElement).style.zIndex = '0'; }}
                >
                  {/* Hotspot center highlight */}
                  {vel > 0.6 && (
                    <div style={{
                      position: 'absolute',
                      top: '50%',
                      left: '50%',
                      transform: 'translate(-50%, -50%)',
                      width: '30%',
                      height: '30%',
                      borderRadius: '50%',
                      background: 'rgba(255,255,255,0.6)',
                      filter: 'blur(2px)',
                      opacity: isCurrentlyPlaying ? 0.9 : vel * 0.5,
                    }} />
                  )}

                  {notesAtPosition.length > 1 && (
                    <div style={{
                      position: 'absolute',
                      top: '-3px',
                      right: '-3px',
                      background: '#fff',
                      color: '#000',
                      fontSize: '7px',
                      borderRadius: '50%',
                      width: '10px',
                      height: '10px',
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'center',
                      fontWeight: 700,
                      zIndex: 10,
                    }}>
                      {notesAtPosition.length}
                    </div>
                  )}

                  {/* Tooltip */}
                  <div className="opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none" style={{
                    position: 'absolute',
                    bottom: '100%',
                    left: '50%',
                    transform: 'translateX(-50%)',
                    marginBottom: '6px',
                    background: '#000',
                    color: '#fff',
                    fontSize: '9px',
                    borderRadius: '4px',
                    padding: '3px 6px',
                    whiteSpace: 'nowrap',
                    zIndex: 50,
                  }}>
                    <div style={{ fontWeight: 600 }}>{getMidiNoteName(getPlaybackMidiNote(primaryNote))}</div>
                    <div>V{primaryNote.track + 1}: {tracks[primaryNote.track]?.name || `Track ${primaryNote.track + 1}`}</div>
                    <div>Vel: {Math.round(primaryNote.velocity * 127)}</div>
                    {notesAtPosition.length > 1 && (
                      <div style={{ color: '#fbbf24' }}>+{notesAtPosition.length - 1} more</div>
                    )}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>
    </div>
  );
};