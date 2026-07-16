import React, { useRef, useState } from 'react';
import { Upload } from 'lucide-react';

interface FileUploadProps {
  onFileSelect: (file: File) => void;
  isLoading: boolean;
  compact?: boolean;
}

export const FileUpload: React.FC<FileUploadProps> = ({ onFileSelect, isLoading }) => {
  const fileInputRef = useRef<HTMLInputElement>(null);
  const [isDragOver, setIsDragOver] = useState(false);

  const handleDragOver = (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragOver(true);
  };

  const handleDragLeave = (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragOver(false);
  };

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault();
    setIsDragOver(false);
    
    const files = Array.from(e.dataTransfer.files);
    const midiFile = files.find(file => file.name.toLowerCase().endsWith('.mid'));
    
    if (midiFile) {
      onFileSelect(midiFile);
    }
  };

  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (file) {
      onFileSelect(file);
    }
  };

  const handleClick = () => {
    fileInputRef.current?.click();
  };

  return (
    <div
      style={{
        position: 'relative',
        display: 'flex',
        alignItems: 'center',
        gap: '1rem',
        padding: '0.8rem 1.2rem',
        borderRadius: '12px',
        cursor: 'pointer',
        background: isDragOver
          ? 'linear-gradient(135deg, rgba(46,185,255,0.15) 0%, rgba(140,0,255,0.1) 100%)'
          : 'linear-gradient(135deg, rgba(46,185,255,0.06) 0%, rgba(140,0,255,0.04) 100%)',
        border: isDragOver
          ? '1.5px solid rgba(46,185,255,0.5)'
          : '1.5px solid rgba(255,255,255,0.1)',
        transition: 'all 0.25s ease',
        overflow: 'hidden',
      }}
      onDragOver={handleDragOver}
      onDragLeave={handleDragLeave}
      onDrop={handleDrop}
      onClick={handleClick}
      onMouseEnter={(e) => {
        e.currentTarget.style.border = '1.5px solid rgba(46,185,255,0.4)';
        e.currentTarget.style.background = 'linear-gradient(135deg, rgba(46,185,255,0.12) 0%, rgba(140,0,255,0.08) 100%)';
      }}
      onMouseLeave={(e) => {
        if (!isDragOver) {
          e.currentTarget.style.border = '1.5px solid rgba(255,255,255,0.1)';
          e.currentTarget.style.background = 'linear-gradient(135deg, rgba(46,185,255,0.06) 0%, rgba(140,0,255,0.04) 100%)';
        }
      }}
    >
      {/* Subtle animated gradient shimmer */}
      <div style={{
        position: 'absolute',
        inset: 0,
        background: 'linear-gradient(90deg, transparent 0%, rgba(46,185,255,0.03) 50%, transparent 100%)',
        animation: 'shimmer 3s ease-in-out infinite',
        pointerEvents: 'none',
      }} />

      <input
        ref={fileInputRef}
        type="file"
        accept=".mid,.midi"
        onChange={handleFileChange}
        style={{ display: 'none' }}
        disabled={isLoading}
      />

      {isLoading ? (
        <div style={{
          display: 'flex',
          alignItems: 'center',
          gap: '0.75rem',
          position: 'relative',
          zIndex: 1,
        }}>
          <div style={{
            width: '20px',
            height: '20px',
            border: '2px solid rgba(46,185,255,0.2)',
            borderTop: '2px solid rgba(46,185,255,0.8)',
            borderRadius: '50%',
            animation: 'spin 0.8s linear infinite',
          }} />
          <span style={{
            fontFamily: 'var(--font-tech)',
            fontSize: '0.75rem',
            color: 'rgba(46,185,255,0.8)',
            textTransform: 'uppercase',
            letterSpacing: '0.06em',
          }}>Parsing...</span>
        </div>
      ) : (
        <div style={{
          display: 'flex',
          alignItems: 'center',
          gap: '0.75rem',
          position: 'relative',
          zIndex: 1,
          width: '100%',
        }}>
          {/* Icon with glow */}
          <div style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            width: '32px',
            height: '32px',
            borderRadius: '8px',
            background: 'rgba(46,185,255,0.12)',
            border: '1px solid rgba(46,185,255,0.2)',
            flexShrink: 0,
          }}>
            <Upload style={{ width: '16px', height: '16px', color: 'rgba(46,185,255,0.8)' }} />
          </div>

          {/* Text */}
          <div style={{ flex: 1 }}>
            <div style={{
              fontFamily: 'var(--font-tech)',
              fontSize: '0.8rem',
              color: 'rgba(255,255,255,0.85)',
              textTransform: 'uppercase',
              letterSpacing: '0.06em',
              lineHeight: 1.2,
            }}>
              Drop or click to load <span style={{ color: 'rgba(46,185,255,0.9)' }}>.mid</span>
            </div>
            <div style={{
              fontFamily: 'var(--font-tech)',
              fontSize: '0.6rem',
              color: 'rgba(255,255,255,0.35)',
              textTransform: 'uppercase',
              letterSpacing: '0.04em',
              marginTop: '2px',
            }}>
              MIDI to TŒRN pattern converter
            </div>
          </div>

          {/* Keyboard shortcut hint */}
          <div style={{
            display: 'flex',
            alignItems: 'center',
            gap: '0.3rem',
            flexShrink: 0,
          }}>
            <span style={{
              fontFamily: 'var(--font-tech)',
              fontSize: '0.55rem',
              color: 'rgba(255,255,255,0.25)',
              textTransform: 'uppercase',
              letterSpacing: '0.04em',
              padding: '2px 6px',
              borderRadius: '4px',
              border: '1px solid rgba(255,255,255,0.1)',
              background: 'rgba(255,255,255,0.03)',
            }}>browse</span>
          </div>
        </div>
      )}

      <style>{`
        @keyframes shimmer {
          0%, 100% { transform: translateX(-100%); }
          50% { transform: translateX(100%); }
        }
        @keyframes spin {
          to { transform: rotate(360deg); }
        }
      `}</style>
    </div>
  );
};