/**
 * Tone list component
 */

import type { S330Tone } from '@/core/midi/S330Client';
import { cn, formatS330Number } from '@/lib/utils';

interface ToneListProps {
  tones: S330Tone[];
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
  const namedCount = tones.filter((t) => !isToneEmpty(t)).length;

  return (
    <div className="card p-2">
      <div className="flex items-center justify-between px-2 py-1 mb-2">
        <span className="text-sm font-medium text-s330-text">
          Tones ({namedCount} of {tones.length} used)
        </span>
      </div>
      <div className="max-h-[500px] overflow-y-auto space-y-1">
        {tones.map((tone, index) => {
          const isEmpty = isToneEmpty(tone);
          return (
            <button
              key={index}
              onClick={() => onSelect(index === selectedIndex ? null : index)}
              className={cn(
                'w-full px-3 py-2 rounded text-left text-sm transition-colors',
                'hover:bg-s330-accent/50',
                index === selectedIndex
                  ? 'bg-s330-highlight text-white'
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
                  isEmpty && 'italic'
                )}>
                  {isEmpty ? '(empty)' : tone.name}
                </span>
              </div>
            </button>
          );
        })}
      </div>
    </div>
  );
}
