import { Midi } from '@tonejs/midi';
import type { MidiNote, TrackInfo, GridNote } from '../types/midi';

// Generate distinct colors for tracks
const generateTrackColors = (count: number): string[] => {
  const colors = [
    'rgb(139, 0, 0)',      // Dark Red
    'rgb(255, 69, 0)',     // Burnt Orange
    'rgb(255, 255, 0)',    // Gold
    'rgb(0, 139, 0)',      // Green
    'rgb(0, 140, 130)',    // Türkis
    'rgb(0, 0, 255)',      // Blue
    'rgb(140, 0, 120)',    // Purple
    'rgb(220, 100, 100)',  // Pink
    'rgb(0, 0, 0)',        // Black
    'rgb(0, 0, 0)',        // Black (s4)
    'rgb(120, 120, 120)',  // Gray (s3)
    'rgb(0, 0, 0)',        // Black
    'rgb(0, 255, 70)',     // Bright Green (s2)
    'rgb(255, 50, 50)'     // Bright Red (s1)
  ];
  
  if (count <= colors.length) {
    return colors.slice(0, count);
  }
  
  // Generate additional colors if needed
  const additionalColors = [];
  for (let i = colors.length; i < count; i++) {
    const hue = (i * 137.508) % 360; // Golden angle approximation
    additionalColors.push(`hsl(${hue}, 70%, 60%)`);
  }
  
  return [...colors, ...additionalColors];
};

export const parseMidiFile = async (file: File): Promise<{ 
  notes: MidiNote[],
  tracks: TrackInfo[],
  bpm: number,
  ticksPerQuarter: number
}> => {
  const arrayBuffer = await file.arrayBuffer();
  const midi = new Midi(arrayBuffer);
  
  const allNotes: MidiNote[] = [];
  const trackInfos: TrackInfo[] = [];
  const colors = generateTrackColors(midi.tracks.length);
  
  let internalTrackId = 0; // Internal track numbering starting from 0
  
  midi.tracks.forEach((track, trackIndex) => {
    const trackNotes = track.notes.map(note => ({
      track: internalTrackId, // Use internal numbering
      note: note.midi,
      velocity: note.velocity,
      time: note.time,
      duration: note.duration
    }));
    
    // Only add tracks that have notes
    if (trackNotes.length > 0) {
      allNotes.push(...trackNotes);
      
      trackInfos.push({
        id: internalTrackId, // Use internal numbering
        name: track.name || `Track ${trackIndex + 1}`,
        color: colors[internalTrackId], // Use internal numbering for colors
        noteCount: trackNotes.length
      });
      
      internalTrackId++; // Increment only for tracks with notes
    }
  });
  
  // Get BPM from the first tempo change, or default to 120
  const bpm = midi.header.tempos.length > 0 ? midi.header.tempos[0].bpm : 120;
  const ticksPerQuarter = midi.header.ppq || 480;
  
  return { notes: allNotes, tracks: trackInfos, bpm, ticksPerQuarter };
};

/**
 * Map MIDI notes to TŒRN grid positions.
 *
 * TŒRN voice mapping (based on device behavior):
 *   Voice 1, Row 2 = C3 (MIDI 48) - base note
 *   Voice 1, Row 1 = B2 (MIDI 47) - one semitone below base
 *   Voice 1, Row 15 = C#4 (MIDI 61)
 *   Each subsequent voice shifts down by 1 semitone.
 *   (e.g., Voice 3 Row 4 = C3)
 *
 * Formula: row = midiNote - 47 + channel
 *   where channel = track + 1 (voices 1-8)
 *
 * Valid rows are 1-15 (15 pitches per voice). Row 16 is reserved.
 * If a note falls outside this range, transpose by octave(s).
 */
export const mapNotesToGrid = (
  notes: MidiNote[], 
  bpm: number = 120, 
  subdivision: number = 16, // 16th notes by default
  timeOffset: number = 0,
  transposeOutOfRange: boolean = true
): GridNote[] => {
  if (notes.length === 0) return [];
  
  // Find the time range
  const minTime = Math.min(...notes.map(n => n.time)) + timeOffset;
  const maxTime = Math.max(...notes.map(n => n.time));
  
  // Calculate musical timing
  // subdivision: 4 = quarter notes, 8 = eighth notes, 16 = sixteenth notes, etc.
  const beatsPerSecond = bpm / 60;
  const subdivisionPerSecond = beatsPerSecond * (subdivision / 4);
  
  // Calculate how many pages we need (16 subdivisions per page)
  const stepsPerPage = 16;
  const timeRange = maxTime - minTime;
  
  // Calculate total steps based on musical timing
  const totalSteps = Math.max(16, Math.ceil(timeRange * subdivisionPerSecond));
  
  // Each step represents this much time
  const stepTime = timeRange > 0 ? timeRange / totalSteps : 1;
  
  const gridNotes: GridNote[] = [];
  
  // Create a grid to track occupied positions per track
  const occupiedPositions = new Map<string, boolean>();
  
  // TŒRN constants
  const MIN_ROW = 1;
  const MAX_ROW = 15; // 15 pitches per voice (row 16 reserved)
  
  // Base MIDI note: Voice 1 Row 1 = B2 (MIDI 47), Row 2 = C3 (MIDI 48)
  const BASE_MIDI_NOTE = 47;
  
  // Process each note
  notes.forEach(note => {
    // Calculate absolute step position in the entire timeline
    const adjustedTime = note.time + timeOffset;
    const absoluteStep = Math.floor((adjustedTime - minTime) / stepTime);
    
    // Calculate which page this step belongs to
    const page = Math.floor(absoluteStep / stepsPerPage);
    
    // Calculate x position within the page (0-15)
    const x = absoluteStep % stepsPerPage;
    
    // TŒRN channel is track + 1 (tracks 0-7 become channels 1-8)
    const channel = note.track + 1;
    
    // Calculate row: Voice 1 Row 2 = C3 (MIDI 48), Row 1 = B2 (MIDI 47)
    // row = midiNote - 47 + channel
    const originalMidiNote = note.note;
    let playbackMidiNote = originalMidiNote;
    let row = originalMidiNote - BASE_MIDI_NOTE + channel;
    
    if (transposeOutOfRange) {
      // Transpose by octaves until within valid range [1, 15]
      // If too high, transpose down; if too low, transpose up
      while (row > MAX_ROW) {
        playbackMidiNote -= 12;
        row = playbackMidiNote - BASE_MIDI_NOTE + channel;
      }
      while (row < MIN_ROW) {
        playbackMidiNote += 12;
        row = playbackMidiNote - BASE_MIDI_NOTE + channel;
      }
      
      // Final safety check (should always pass after transposition)
      if (row < MIN_ROW || row > MAX_ROW) {
        return; // Skip if still out of bounds
      }
    } else {
      // Clamp grid position only; keep original pitch for playback
      if (row < MIN_ROW || row > MAX_ROW) {
        row = Math.max(MIN_ROW, Math.min(MAX_ROW, row));
      }
    }
    
    // Convert to 0-indexed Y for grid (row 1 = y 0, row 15 = y 14)
    const finalY = row - 1;
    
    const positionKey = `${note.track}-${page}-${x}-${finalY}`;
    
    // Only add if this position isn't already occupied by this track
    if (!occupiedPositions.has(positionKey)) {
      occupiedPositions.set(positionKey, true);
      
      gridNotes.push({
        track: note.track,
        note: playbackMidiNote,
        velocity: note.velocity,
        time: note.time,
        duration: note.duration,
        x: x,
        y: finalY,
        page: page
      });
    }
  });
  
  return gridNotes;
};