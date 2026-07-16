export interface MidiNote {
  track: number;
  note: number;
  velocity: number;
  time: number;
  duration: number;
}

export interface TrackInfo {
  id: number;
  name: string;
  color: string;
  noteCount: number;
}

export interface GridNote extends MidiNote {
  x: number;
  y: number;
  page: number;
}