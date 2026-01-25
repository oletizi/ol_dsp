/**
 * Tone list component
 */

import type { ToneNameInfo } from '@/core/midi/S330Client';
import { cn } from '@/lib/utils';

interface ToneListProps {
  toneNames: ToneNameInfo[];
  selectedIndex: number | null;
  onSelect: (index: number | null) => void;
}

export function ToneList({ toneNames, selectedIndex, onSelect }: ToneListProps) {
  const namedCount = toneNames.filter((t) => !t.isEmpty).length;

  return (
    <div className="card p-2">
      <div className="flex items-center justify-between px-2 py-1 mb-2">
        <span className="text-sm font-medium text-s330-text">
          Tones ({namedCount} of {toneNames.length} used)
        </span>
      </div>
      <div className="max-h-[500px] overflow-y-auto space-y-1">
        {toneNames.map((tone) => (
          <button
            key={tone.index}
            onClick={() => onSelect(tone.index === selectedIndex ? null : tone.index)}
            className={cn(
              'w-full px-3 py-2 rounded text-left text-sm transition-colors',
              'hover:bg-s330-accent/50',
              tone.index === selectedIndex
                ? 'bg-s330-highlight text-white'
                : tone.isEmpty
                  ? 'text-s330-muted/50'
                  : 'text-s330-text'
            )}
          >
            <div className="flex items-center justify-between">
              <span className="font-mono">
                T{tone.index + 11}
              </span>
              <span className={cn(
                'flex-1 mx-3 truncate',
                tone.isEmpty && 'italic'
              )}>
                {tone.isEmpty ? '(empty)' : tone.name}
              </span>
            </div>
          </button>
        ))}
      </div>
    </div>
  );
}
