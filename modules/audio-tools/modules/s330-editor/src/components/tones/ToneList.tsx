/**
 * Tone list component - supports sparse arrays for partial loading
 */

import type { S330Tone } from '@/core/midi/S330Client';
import { cn, formatS330Number } from '@/lib/utils';

interface ToneListProps {
  tones: (S330Tone | undefined)[];
  selectedIndex: number | null;
  onSelect: (index: number | null) => void;
}

/**
 * Check if a tone is considered "empty" (no meaningful data)
 */
function isToneEmpty(tone: S330Tone): boolean {
  const name = tone.name.trim();
  return name === '' || name === '        ';
}

export function ToneList({ tones, selectedIndex, onSelect }: ToneListProps) {
  const loadedTones = tones.filter((t): t is S330Tone => t !== undefined);
  const namedCount = loadedTones.filter((t) => !isToneEmpty(t)).length;

  return (
    <div className="card p-2">
      <div className="flex items-center justify-between px-2 py-1 mb-2">
        <span className="text-sm font-medium text-s330-text">
          Tones ({namedCount} used)
        </span>
      </div>
      <div className="max-h-[500px] overflow-y-auto space-y-1">
        {tones.map((tone, index) => {
          const isLoaded = tone !== undefined;
          const isEmpty = isLoaded && isToneEmpty(tone);
          return (
            <button
              key={index}
              onClick={() => isLoaded && onSelect(index === selectedIndex ? null : index)}
              disabled={!isLoaded}
              className={cn(
                'w-full px-3 py-2 rounded text-left text-sm transition-colors',
                isLoaded && 'hover:bg-s330-accent/50',
                index === selectedIndex
                  ? 'bg-s330-highlight text-white'
                  : !isLoaded
                    ? 'text-s330-muted/30 cursor-not-allowed'
                    : isEmpty
                      ? 'text-s330-muted/50'
                      : 'text-s330-text'
              )}
            >
              <div className="flex items-center justify-between">
                <span className="font-mono">
                  T{formatS330Number(index)}
                </span>
                <span className={cn(
                  'flex-1 mx-3 truncate',
                  (isEmpty || !isLoaded) && 'italic'
                )}>
                  {!isLoaded ? '(not loaded)' : isEmpty ? '(empty)' : tone.name}
                </span>
              </div>
            </button>
          );
        })}
      </div>
    </div>
  );
}
