/**
 * Patch list component - supports sparse arrays for partial loading
 */

import type { S330Patch } from '@/core/midi/S330Client';
import { cn, formatS330Number } from '@/lib/utils';

interface PatchListProps {
  patches: (S330Patch | undefined)[];
  selectedIndex: number | null;
  onSelect: (index: number | null) => void;
}

/**
 * Check if a patch is considered "empty" (no meaningful data)
 */
function isPatchEmpty(patch: S330Patch): boolean {
  const name = patch.common.name.trim();
  return name === '' || name === '            ';
}

export function PatchList({ patches, selectedIndex, onSelect }: PatchListProps) {
  const loadedPatches = patches.filter((p): p is S330Patch => p !== undefined);
  const namedCount = loadedPatches.filter((p) => !isPatchEmpty(p)).length;

  return (
    <div className="card p-2">
      <div className="flex items-center justify-between px-2 py-1 mb-2">
        <span className="text-sm font-medium text-s330-text">
          Patches ({namedCount} used)
        </span>
      </div>
      <div className="max-h-[500px] overflow-y-auto space-y-1">
        {patches.map((patch, index) => {
          const isLoaded = patch !== undefined;
          const isEmpty = isLoaded && isPatchEmpty(patch);
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
                  P{formatS330Number(index)}
                </span>
                <span className={cn(
                  'flex-1 mx-3 truncate',
                  (isEmpty || !isLoaded) && 'italic'
                )}>
                  {!isLoaded ? '(not loaded)' : isEmpty ? '(empty)' : patch.common.name}
                </span>
              </div>
            </button>
          );
        })}
      </div>
    </div>
  );
}
