/**
 * Play page - Main overview similar to S-330's PLAY screen
 *
 * Shows 8 MIDI parts (A-H) with their patch assignments,
 * output routing, and levels.
 * Loads first bank (8 patches) by default for faster startup.
 */

import { useEffect, useState, useRef, useCallback } from 'react';
import { Link } from 'react-router-dom';
import { useMidiStore } from '@/stores/midiStore';
import { useS330Store } from '@/stores/s330Store';
import { createS330Client } from '@/core/midi/S330Client';
import type { S330ClientInterface, S330Patch } from '@/core/midi/S330Client';
import { cn, formatS330Number } from '@/lib/utils';

// Global flag to prevent React Strict Mode from running effects twice
// This is necessary because MIDI requests can't be safely run concurrently
let isFetchingGlobal = false;

// Constants for bank loading (S-330 uses banks of 8)
const PATCHES_PER_BANK = 8;
const TOTAL_PATCHES = 16;  // 2 banks of 8

// MIDI Part configuration (A-H = channels 1-8)
interface MidiPart {
  id: string;
  channel: number;
  patchIndex: number | null;
  patchName: string;
  output: number;
  level: number;
  active: boolean;
}

const PART_LABELS = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'];

/**
 * Check if a patch is considered "empty" (no meaningful data)
 */
function isPatchEmpty(patch: S330Patch): boolean {
  const name = patch.common.name.trim();
  return name === '' || name === '            ';
}

export function PlayPage() {
  const { adapter, deviceId, status } = useMidiStore();
  const {
    setLoading,
    setError,
    setProgress,
    clearProgress,
    isLoading,
    loadingProgress,
    loadingMessage,
    error,
  } = useS330Store();

  const isConnected = status === 'connected' && adapter !== null;

  // Keep a ref to the S330 client for sending parameter updates
  const clientRef = useRef<S330ClientInterface | null>(null);

  // Local state for patches (sparse array - undefined = not loaded)
  const [patches, setPatches] = useState<(S330Patch | undefined)[]>([]);
  const [loadedPatchBanks, setLoadedPatchBanks] = useState<Set<number>>(new Set());

  // MIDI parts - loaded from S-330 function parameters
  const [parts, setParts] = useState<MidiPart[]>(
    PART_LABELS.map((id, i) => ({
      id,
      channel: i, // 0-15 (Ch 1-16)
      patchIndex: null,
      patchName: '',
      output: 1, // 1-8 for individual outputs
      level: 127,
      active: true,
    }))
  );
  const [partsLoaded, setPartsLoaded] = useState(false);

  // Initialize client when adapter changes
  useEffect(() => {
    if (!adapter) {
      clientRef.current = null;
      return;
    }
    clientRef.current = createS330Client(adapter, { deviceId });
  }, [adapter, deviceId]);

  // Load a specific bank of patches (updates UI progressively)
  const loadPatchBank = useCallback(async (bankIndex: number, forceReload = false) => {
    if (!clientRef.current) return;

    const startIndex = bankIndex * PATCHES_PER_BANK;
    const count = PATCHES_PER_BANK;

    try {
      setLoading(true, `${forceReload ? 'Reloading' : 'Loading'} patches ${startIndex + 1}-${startIndex + count}...`);
      setError(null);

      // Ensure array is large enough before loading
      setPatches((prev) => {
        if (prev.length >= TOTAL_PATCHES) return prev;
        const updated = [...prev];
        while (updated.length < TOTAL_PATCHES) updated.push(undefined);
        return updated;
      });

      await clientRef.current.connect();
      await clientRef.current.loadPatchRange(
        startIndex,
        count,
        (current, total) => setProgress(current, total),
        // Update UI immediately when each patch is loaded
        (index, patch) => {
          setPatches((prev) => {
            const updated = [...prev];
            updated[index] = patch;
            return updated;
          });
        },
        forceReload
      );

      setLoadedPatchBanks((prev) => new Set([...prev, bankIndex]));
      clearProgress();
      setLoading(false);
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Failed to load patches';
      setError(message);
      clearProgress();
      setLoading(false);
    }
  }, [setLoading, setError, setProgress, clearProgress]);

  // Load function parameters (multi-mode configuration)
  const loadFunctionParams = useCallback(async () => {
    if (!clientRef.current) return;

    try {
      setLoading(true, 'Loading multi-mode configuration...');
      setError(null);

      await clientRef.current.connect();
      const configs = await clientRef.current.requestFunctionParameters();

      // Update parts with loaded configuration
      setParts((prev) =>
        prev.map((part, i) => ({
          ...part,
          channel: configs[i]?.channel ?? i,
          patchIndex: configs[i]?.patchIndex ?? null,
          output: configs[i]?.output ?? 1,
          level: configs[i]?.level ?? 127,
        }))
      );
      setPartsLoaded(true);

      clearProgress();
      setLoading(false);
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Failed to load configuration';
      setError(message);
      clearProgress();
      setLoading(false);
    }
  }, [setLoading, setError, clearProgress]);

  // Load initial data (function params + first bank of 8 patches)
  const loadInitialData = useCallback(async () => {
    if (isFetchingGlobal) return;
    isFetchingGlobal = true;

    try {
      await loadFunctionParams();
      await loadPatchBank(0);
    } finally {
      isFetchingGlobal = false;
    }
  }, [loadFunctionParams, loadPatchBank]);

  // Auto-load initial data when connected
  useEffect(() => {
    if (!adapter) {
      clientRef.current = null;
      return;
    }

    // Skip if already fetching or data already loaded
    if (isFetchingGlobal || partsLoaded) {
      return;
    }

    loadInitialData();
  }, [adapter, partsLoaded, loadInitialData]);

  // Update patch names based on patchIndex loaded from function parameters
  useEffect(() => {
    const loadedPatches = patches.filter((p): p is S330Patch => p !== undefined);
    if (loadedPatches.length === 0) return;

    // Look up patch names based on patchIndex from function parameters
    setParts((prev) =>
      prev.map((part) => {
        if (part.patchIndex !== null && patches[part.patchIndex] !== undefined) {
          const patch = patches[part.patchIndex]!;
          if (!isPatchEmpty(patch)) {
            return { ...part, patchName: patch.common.name };
          }
        }
        return { ...part, patchName: '' };
      })
    );
  }, [patches]);

  // Handle field updates
  const updatePart = (partIndex: number, updates: Partial<MidiPart>) => {
    setParts((prev) =>
      prev.map((part, i) => (i === partIndex ? { ...part, ...updates } : part))
    );
  };

  const handleChannelChange = (partIndex: number, channel: number) => {
    updatePart(partIndex, { channel });

    // Send parameter update to hardware
    if (clientRef.current) {
      try {
        clientRef.current.setMultiChannel(partIndex, channel);
      } catch (err) {
        console.error('[PlayPage] Failed to send channel parameter:', err);
        setError(
          err instanceof Error ? err.message : 'Failed to update channel'
        );
      }
    }
  };

  const handlePatchChange = (partIndex: number, patchIndex: number | null) => {
    const patch = patchIndex !== null ? patches[patchIndex] : undefined;
    updatePart(partIndex, {
      patchIndex,
      patchName: patch && !isPatchEmpty(patch) ? patch.common.name : '',
    });

    // Send parameter update to hardware
    if (clientRef.current) {
      try {
        clientRef.current.setMultiPatch(partIndex, patchIndex);
      } catch (err) {
        console.error('[PlayPage] Failed to send patch parameter:', err);
        setError(err instanceof Error ? err.message : 'Failed to update patch');
      }
    }
  };

  const handleOutputChange = (partIndex: number, output: number) => {
    updatePart(partIndex, { output });

    // Send parameter update to hardware
    if (clientRef.current) {
      try {
        clientRef.current.setMultiOutput(partIndex, output);
      } catch (err) {
        console.error('[PlayPage] Failed to send output parameter:', err);
        setError(
          err instanceof Error ? err.message : 'Failed to update output'
        );
      }
    }
  };

  const handleLevelChange = (partIndex: number, level: number) => {
    updatePart(partIndex, { level });

    // Send parameter update to hardware
    if (clientRef.current) {
      try {
        clientRef.current.setMultiLevel(partIndex, level);
      } catch (err) {
        console.error('[PlayPage] Failed to send level parameter:', err);
        setError(err instanceof Error ? err.message : 'Failed to update level');
      }
    }
  };

  // Reload data from hardware
  const handleReload = useCallback(async () => {
    if (!clientRef.current) return;

    // Invalidate cache and reset state
    clientRef.current.invalidatePatchCache();
    setPatches([]);
    setLoadedPatchBanks(new Set());
    setPartsLoaded(false);
    await loadInitialData();
  }, [loadInitialData]);

  const loadedPatchCount = patches.filter(p => p !== undefined).length;

  if (!isConnected) {
    return (
      <div className="card text-center py-12">
        <h2 className="text-xl font-bold text-s330-text mb-2">Not Connected</h2>
        <p className="text-s330-muted mb-4">
          Connect to your S-330 to view the play screen.
        </p>
        <Link to="/" className="btn btn-primary inline-block">
          Go to Connection
        </Link>
      </div>
    );
  }

  return (
    <div className="space-y-4">
      {/* Header */}
      <div className="flex items-center justify-between gap-4">
        <div className="flex items-center gap-4">
          <h2 className="text-xl font-bold text-s330-text">Play</h2>
          <span className="text-sm text-s330-muted">
            {loadedPatchCount} of {TOTAL_PATCHES} patches loaded
          </span>
        </div>
        <div className="flex items-center gap-4 flex-1 justify-end">
          {/* Loading Progress (inline with buttons) */}
          {isLoading && loadingProgress !== null && (
            <div className="flex-1 max-w-xs">
              <div className="h-2 bg-s330-bg rounded-full overflow-hidden">
                <div
                  className="h-full bg-s330-highlight transition-all duration-150 ease-out"
                  style={{ width: `${loadingProgress}%` }}
                />
              </div>
              <p className="text-s330-muted text-xs mt-0.5 truncate">{loadingMessage}</p>
            </div>
          )}
          <div className="flex gap-2">
            <button
              onClick={() => loadPatchBank(0, loadedPatchBanks.has(0))}
              disabled={isLoading}
              className={cn('btn btn-primary', isLoading && 'opacity-50')}
            >
              {loadedPatchBanks.has(0) ? 'Reload' : 'Load'} 1-8
            </button>
            <button
              onClick={() => loadPatchBank(1, loadedPatchBanks.has(1))}
              disabled={isLoading}
              className={cn('btn btn-primary', isLoading && 'opacity-50')}
            >
              {loadedPatchBanks.has(1) ? 'Reload' : 'Load'} 9-16
            </button>
            <button
              onClick={handleReload}
              disabled={isLoading}
              className={cn('btn btn-secondary', isLoading && 'opacity-50')}
            >
              {isLoading ? 'Loading...' : 'Refresh All'}
            </button>
          </div>
        </div>
      </div>

      {/* Error Display */}
      {error && (
        <div className="bg-red-500/20 border border-red-500 rounded-md p-3">
          <p className="text-red-200 text-sm">{error}</p>
        </div>
      )}

      {/* Parts Grid */}
      <div className="bg-s330-panel border border-s330-accent rounded-md overflow-hidden">
        {/* Parts Grid */}
        <div className="p-4 font-mono text-sm">
          {/* Header row */}
          <div className="grid grid-cols-12 gap-2 mb-2 text-s330-muted text-xs">
            <div className="col-span-1"></div>
            <div className="col-span-1">VAL</div>
            <div className="col-span-1 text-center">CH</div>
            <div className="col-span-4">Patch</div>
            <div className="col-span-1 text-center">Out</div>
            <div className="col-span-4">Level</div>
          </div>

          {/* Part rows */}
          {parts.map((part, index) => (
            <div
              key={part.id}
              className="grid grid-cols-12 gap-2 py-1.5 px-1 rounded transition-colors hover:bg-s330-accent/10"
            >
              {/* Part label */}
              <div className="col-span-1 text-s330-highlight font-bold">
                {part.id}
              </div>

              {/* VAL (active indicator) */}
              <div className="col-span-1 text-s330-text">
                {part.active ? '*' : ''}
              </div>

              {/* Channel - editable dropdown (0-15 stored, display as 1-16) */}
              <div className="col-span-1 text-center">
                <select
                  value={part.channel}
                  onChange={(e) => handleChannelChange(index, Number(e.target.value))}
                  onClick={(e) => e.stopPropagation()}
                  className={cn(
                    'w-full px-1 py-0.5 text-center text-xs font-mono',
                    'bg-s330-panel border border-s330-accent rounded',
                    'text-s330-text hover:bg-s330-accent/30',
                    'focus:outline-none focus:ring-1 focus:ring-s330-highlight'
                  )}
                >
                  {Array.from({ length: 16 }, (_, i) => (
                    <option key={i} value={i}>
                      {i + 1}
                    </option>
                  ))}
                </select>
              </div>

              {/* Patch - editable dropdown */}
              <div className="col-span-4">
                <select
                  value={part.patchIndex ?? -1}
                  onChange={(e) => {
                    const value = Number(e.target.value);
                    handlePatchChange(index, value === -1 ? null : value);
                  }}
                  onClick={(e) => e.stopPropagation()}
                  className={cn(
                    'w-full px-1 py-0.5 text-xs font-mono',
                    'bg-s330-panel border border-s330-accent rounded',
                    'text-s330-text hover:bg-s330-accent/30',
                    'focus:outline-none focus:ring-1 focus:ring-s330-highlight'
                  )}
                >
                  <option value={-1} className="text-s330-muted">
                    ---
                  </option>
                  {Array.from({ length: TOTAL_PATCHES }, (_, patchIndex) => {
                    const patch = patches[patchIndex];
                    const isLoaded = patch !== undefined;
                    return (
                      <option key={patchIndex} value={patchIndex} disabled={!isLoaded}>
                        P{formatS330Number(patchIndex)} {!isLoaded ? '(not loaded)' : isPatchEmpty(patch) ? '(empty)' : patch.common.name}
                      </option>
                    );
                  })}
                </select>
              </div>

              {/* Output - editable dropdown (1-8 for individual outputs) */}
              <div className="col-span-1 text-center">
                <select
                  value={part.output}
                  onChange={(e) => handleOutputChange(index, Number(e.target.value))}
                  onClick={(e) => e.stopPropagation()}
                  className={cn(
                    'w-full px-1 py-0.5 text-center text-xs font-mono',
                    'bg-s330-panel border border-s330-accent rounded',
                    'text-s330-text hover:bg-s330-accent/30',
                    'focus:outline-none focus:ring-1 focus:ring-s330-highlight'
                  )}
                >
                  {Array.from({ length: 8 }, (_, i) => (
                    <option key={i + 1} value={i + 1}>
                      {i + 1}
                    </option>
                  ))}
                </select>
              </div>

              {/* Level - slider with value display */}
              <div className="col-span-4 flex items-center gap-2">
                <input
                  type="range"
                  min={0}
                  max={127}
                  value={part.level}
                  onChange={(e) => handleLevelChange(index, Number(e.target.value))}
                  onClick={(e) => e.stopPropagation()}
                  className={cn(
                    'flex-1 h-1.5 rounded-full appearance-none cursor-pointer',
                    'bg-s330-accent/50',
                    '[&::-webkit-slider-thumb]:appearance-none',
                    '[&::-webkit-slider-thumb]:w-3',
                    '[&::-webkit-slider-thumb]:h-3',
                    '[&::-webkit-slider-thumb]:rounded-full',
                    '[&::-webkit-slider-thumb]:bg-s330-highlight',
                    '[&::-webkit-slider-thumb]:hover:bg-s330-text',
                    '[&::-moz-range-thumb]:w-3',
                    '[&::-moz-range-thumb]:h-3',
                    '[&::-moz-range-thumb]:rounded-full',
                    '[&::-moz-range-thumb]:bg-s330-highlight',
                    '[&::-moz-range-thumb]:border-0',
                    '[&::-moz-range-thumb]:hover:bg-s330-text'
                  )}
                />
                <span className="w-8 text-right text-xs text-s330-text font-mono">
                  {part.level}
                </span>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Display section - shows current patches summary */}
      <div className="bg-s330-panel border border-s330-accent rounded-md p-4">
        <div className="text-xs text-s330-muted mb-3 font-mono">
          Active Part Assignments
        </div>
        <div className="grid grid-cols-2 md:grid-cols-4 gap-2 font-mono text-sm">
          {parts.slice(0, 8).map((part) => (
            <div key={part.id} className="text-s330-text">
              {part.patchIndex !== null ? (
                <>
                  <span className="text-s330-muted">P</span>
                  {formatS330Number(part.patchIndex)}{' '}
                  <span className="text-s330-highlight">{part.patchName}</span>
                </>
              ) : (
                <span className="text-s330-muted">
                  P{formatS330Number(part.patchIndex ?? 0)}
                </span>
              )}
            </div>
          ))}
        </div>
      </div>

    </div>
  );
}
