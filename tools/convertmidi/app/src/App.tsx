import React, { useState, useCallback } from 'react';
import { Music2, FileText, Plus } from 'lucide-react';
import { FileUpload } from './components/FileUpload';
import { TrackLegend } from './components/TrackLegend';
import { NoteGrid } from './components/NoteGrid';
import { PageNavigation } from './components/PageNavigation';
import { TimingControls } from './components/TimingControls';
import { PatternDownload } from './components/PatternDownload';
import { parseMidiFile, mapNotesToGrid } from './utils/midiParser';
import type { MidiNote, TrackInfo, GridNote } from './types/midi';

function App() {
  const [notes, setNotes] = useState<GridNote[]>([]);
  const [originalNotes, setOriginalNotes] = useState<MidiNote[]>([]);
  const [tracks, setTracks] = useState<TrackInfo[]>([]);
  const [enabledTracks, setEnabledTracks] = useState<Set<number>>(new Set());
  const [currentPage, setCurrentPage] = useState(0);
  const [isLoading, setIsLoading] = useState(false);
  const [fileName, setFileName] = useState<string>('');
  const [bpm, setBpm] = useState(120);
  const [subdivision, setSubdivision] = useState(8);
  const [timeOffset, setTimeOffset] = useState(0);
  const [exportStartPage, setExportStartPage] = useState(0);
  const [filterOverlapping, setFilterOverlapping] = useState(false);
  const [transposeOutOfRange, setTransposeOutOfRange] = useState(true);
  const [notePriority, setNotePriority] = useState<'highest' | 'lowest'>('highest');
  const [pageNoteCounts, setPageNoteCounts] = useState<{ filtered: number; total: number }>({ filtered: 0, total: 0 });

  // Callback stabilisieren, damit sich die Referenz nicht bei jedem Render ändert
  const handlePageNoteCount = React.useCallback((filtered: number, total: number) => {
    setPageNoteCounts({ filtered, total });
  }, []);

  const addEmptyTrack = useCallback(() => {
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

    const newTrackId = tracks.length;
    const newTrack: TrackInfo = {
      id: newTrackId,
      name: `Empty Track ${newTrackId + 1}`,
      color: colors[newTrackId] || `hsl(${(newTrackId * 137.508) % 360}, 70%, 60%)`,
      noteCount: 0
    };

    setTracks([...tracks, newTrack]);
    // Automatically enable the new empty track
    setEnabledTracks(new Set([...enabledTracks, newTrackId]));
  }, [tracks, enabledTracks]);

  const handleTrackReorder = useCallback((newTracks: TrackInfo[]) => {
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

    // Create a mapping from old track IDs to new track IDs and assign new colors
    const trackIdMapping = new Map<number, number>();
    const reorderedTracksWithNewColors = newTracks.map((track, newIndex) => {
      const oldTrack = tracks.find(t => t.name === track.name);
      if (oldTrack) {
        trackIdMapping.set(oldTrack.id, newIndex);
      }
      return {
        ...track,
        id: newIndex,
        color: colors[newIndex] || `hsl(${(newIndex * 137.508) % 360}, 70%, 60%)`
      };
    });

    // Update tracks
    setTracks(reorderedTracksWithNewColors);

    // Update enabled tracks with new IDs
    const newEnabledTracks = new Set<number>();
    enabledTracks.forEach(oldId => {
      const newId = trackIdMapping.get(oldId);
      if (newId !== undefined) {
        newEnabledTracks.add(newId);
      }
    });
    setEnabledTracks(newEnabledTracks);

    // Update notes with new track IDs
    const updatedOriginalNotes = originalNotes.map(note => ({
      ...note,
      track: trackIdMapping.get(note.track) ?? note.track
    }));
    setOriginalNotes(updatedOriginalNotes);

    // Recalculate grid with updated notes
    const gridNotes = mapNotesToGrid(updatedOriginalNotes, bpm, subdivision, timeOffset, transposeOutOfRange);
    setNotes(gridNotes);
  }, [tracks, enabledTracks, originalNotes, bpm, subdivision, timeOffset, transposeOutOfRange]);

  const recalculateGrid = useCallback(() => {
    if (originalNotes.length === 0) return;
    
    const gridNotes = mapNotesToGrid(originalNotes, bpm, subdivision, timeOffset, transposeOutOfRange);
    setNotes(gridNotes);
    setCurrentPage(0);
  }, [originalNotes, bpm, subdivision, timeOffset, transposeOutOfRange]);

  // Recalculate when timing parameters change
  React.useEffect(() => {
    recalculateGrid();
  }, [recalculateGrid]);

  const handleFileSelect = useCallback(async (file: File) => {
    setIsLoading(true);
    try {
      const { notes: parsedNotes, tracks: parsedTracks, bpm: fileBpm } = await parseMidiFile(file);
      setOriginalNotes(parsedNotes);
      setBpm(fileBpm); // Use the BPM from the MIDI file
      const gridNotes = mapNotesToGrid(parsedNotes, fileBpm, subdivision, 0, transposeOutOfRange);
      
      setNotes(gridNotes);
      setTracks(parsedTracks);
      setEnabledTracks(new Set(parsedTracks.map(track => track.id)));
      setCurrentPage(0);
      setFileName(file.name);
    } catch (error) {
      console.error('Error parsing MIDI file:', error);
      alert('Error parsing MIDI file. Please make sure it\'s a valid .mid file.');
    } finally {
      setIsLoading(false);
    }
  }, [subdivision, transposeOutOfRange]);

  const handleDownloadPattern = useCallback((patternData: Uint8Array) => {
    const blob = new Blob([patternData], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `pattern_page_${currentPage + 1}.txt`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [currentPage]);

  const totalPages = Math.max(1, Math.max(...notes.map(n => n.page), 0) + 1);
  const hasData = notes.length > 0;

  return (
    <div className="min-h-screen">
      <nav className="tool-nav">
        <div className="tool-nav-brand">
          <a href="../../" className="tool-nav-logo">TŒRN</a>
          <span className="tool-nav-label">// MIDI2TŒRN</span>
        </div>
        <div className="tool-nav-meta"></div>
      </nav>

      <main className="tool-shell">
        <div className="tool-content">
          {/* Upload - always compact at top */}
          <div style={{ marginBottom: '1.5rem' }}>
            <FileUpload onFileSelect={handleFileSelect} isLoading={isLoading} compact={true} />
          </div>

          {/* Main Content */}
          <div className="space-y-6">
            {hasData && (
              <>
              {/* Stats */}
              <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
                <div className="bg-gray-800 rounded-xl p-6">
                  <div className="flex items-center space-x-3">
                    <div className="p-2 bg-blue-600/20 rounded-lg">
                      <Music2 className="w-6 h-6 text-blue-400" />
                    </div>
                    <div>
                      <p className="text-2xl font-bold text-white">{notes.length}</p>
                      <p className="text-gray-400">Total Notes</p>
                    </div>
                  </div>
                </div>
                
                <div className="bg-gray-800 rounded-xl p-6">
                  <div className="flex items-center space-x-3">
                    <div className="p-2 bg-green-600/20 rounded-lg">
                      <FileText className="w-6 h-6 text-green-400" />
                    </div>
                    <div>
                      <p className="text-2xl font-bold text-white">{tracks.length}</p>
                      <p className="text-gray-400">Tracks</p>
                    </div>
                  </div>
                </div>

                <div className="bg-gray-800 rounded-xl p-6">
                  <div className="flex items-center space-x-3">
                    <div className="p-2 bg-purple-600/20 rounded-lg">
                      <div className="w-6 h-6 bg-purple-400 rounded grid grid-cols-4 gap-px p-1">
                        {Array.from({ length: 16 }, (_, i) => (
                          <div key={i} className="bg-gray-800 rounded-sm" />
                        ))}
                      </div>
                    </div>
                    <div>
                      <p className="text-2xl font-bold text-white">{totalPages}</p>
                      <p className="text-gray-400">Pages</p>
                    </div>
                  </div>
                </div>
              </div>

              {/* Timing Controls */}
              <TimingControls
                bpm={bpm}
                subdivision={subdivision}
                timeOffset={timeOffset}
                onBpmChange={setBpm}
                onSubdivisionChange={setSubdivision}
                onTimeOffsetChange={setTimeOffset}
                originalNotes={originalNotes}
              />

              {/* Track Legend */}
              <TrackLegend 
                tracks={tracks} 
                enabledTracks={enabledTracks}
                onTrackReorder={handleTrackReorder}
                onAddEmptyTrack={addEmptyTrack}
                onTrackToggle={(trackId, enabled) => {
                  const newEnabledTracks = new Set(enabledTracks);
                  if (enabled) {
                    newEnabledTracks.add(trackId);
                  } else {
                    newEnabledTracks.delete(trackId);
                  }
                  setEnabledTracks(newEnabledTracks);
                }}
              />

              {/* Grid + Page Navigation + Pattern Export side by side */}
              <div style={{ display: 'flex', gap: '1.5rem', alignItems: 'flex-start' }}>
                {/* Note Grid */}
                <div style={{ width: '60%', flexShrink: 0 }}>
                <NoteGrid
                  notes={notes.filter(note => enabledTracks.has(note.track))}
                  tracks={tracks}
                  currentPage={currentPage}
                  totalPages={totalPages}
                  onPageChange={setCurrentPage}
                  onDownloadPattern={handleDownloadPattern}
                  bpm={bpm}
                  subdivision={subdivision}
                  enabledTracks={enabledTracks}
                  filterOverlapping={filterOverlapping}
                  onFilterOverlappingChange={setFilterOverlapping}
                  transposeOutOfRange={transposeOutOfRange}
                  notePriority={notePriority}
                  onNotePriorityChange={setNotePriority}
                  pageNoteCount={handlePageNoteCount}
                />
                </div>

                {/* Controls + Page Navigation + Pattern Export stacked */}
                <div style={{ width: '40%', minWidth: 0, display: 'flex', flexDirection: 'column', gap: '1rem' }}>

                {/* Filter & Note Count */}
                <div className="bg-gray-800 rounded-xl p-3" style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                    <input
                      type="checkbox"
                      id="filter-overlapping"
                      checked={filterOverlapping}
                      onChange={(e) => setFilterOverlapping(e.target.checked)}
                      className="w-4 h-4 text-blue-600 bg-gray-600 border-gray-500 rounded focus:ring-blue-500 focus:ring-2"
                    />
                    <label htmlFor="filter-overlapping" className="text-sm text-gray-300">
                      Filter overlapping
                    </label>
                    {filterOverlapping && (
                      <select
                        value={notePriority}
                        onChange={(e) => setNotePriority(e.target.value as 'highest' | 'lowest')}
                        className="text-xs bg-gray-600 text-white rounded px-2 py-1 border border-gray-500 focus:border-blue-400 focus:outline-none ml-auto"
                      >
                        <option value="highest">Keep highest</option>
                        <option value="lowest">Keep lowest</option>
                      </select>
                    )}
                  </div>
                  <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                    <input
                      type="checkbox"
                      id="transpose-out-of-range"
                      checked={transposeOutOfRange}
                      onChange={(e) => setTransposeOutOfRange(e.target.checked)}
                      className="w-4 h-4 text-blue-600 bg-gray-600 border-gray-500 rounded focus:ring-blue-500 focus:ring-2"
                    />
                    <label htmlFor="transpose-out-of-range" className="text-sm text-gray-300">
                      Transpose out-of-range notes
                    </label>
                  </div>
                  <p className="text-xs text-gray-500">
                    When off, notes outside the grid range keep their original pitch in preview (placed at the nearest row).
                  </p>
                  <div className="text-sm text-gray-400">
                    {pageNoteCounts.filtered} notes on this page
                    {filterOverlapping && pageNoteCounts.filtered !== pageNoteCounts.total && (
                      <span className="text-yellow-400 ml-2">
                        ({pageNoteCounts.total - pageNoteCounts.filtered} filtered)
                      </span>
                    )}
                  </div>
                </div>
                <PageNavigation
                  currentPage={currentPage}
                  totalPages={totalPages}
                  onPageChange={setCurrentPage}
                />

                {/* Pattern Download */}
                <PatternDownload
                  notes={notes.filter(note => enabledTracks.has(note.track))}
                  currentPage={currentPage}
                  exportStartPage={exportStartPage}
                  onExportStartPageChange={setExportStartPage}
                  totalPages={totalPages}
                  fileName={fileName}
                />
                </div>
              </div>


              </>
            )}
          </div>
        </div>
      </main>
    </div>
  );
}

export default App;