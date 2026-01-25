/**
 * Play page - Main overview similar to S-330's PLAY screen
 *
 * Shows 8 MIDI parts (A-H) with their patch assignments,
 * output routing, and levels.
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
  const { setLoading, setError, isLoading, error } = useS330Store();

  const isConnected = status === 'connected' && adapter !== null;

  // Keep a ref to the S330 client for sending parameter updates
  const clientRef = useRef<S330ClientInterface | null>(null);

  // Local state for patches (cached in client, mirrored here for display)
  const [patches, setPatches] = useState<S330Patch[]>([]);

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

  // Fetch all data using the cached API
  const fetchData = useCallback(async () => {
    if (!adapter || isFetchingGlobal) return;

    isFetchingGlobal = true;
    try {
      setLoading(true, 'Loading patches...');

      const client = createS330Client(adapter, { deviceId });
      clientRef.current = client;
      await client.connect();

      // Fetch patches using cached API
      const allPatches = await client.getAllPatches();
      setPatches(allPatches);

      // Then fetch function parameters
      setLoading(true, 'Loading multi mode configuration...');
      const configs = await client.requestFunctionParameters();

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

      setLoading(false);
    } catch (err) {
      console.error('[PlayPage] Error fetching data:', err);
      setError(err instanceof Error ? err.message : 'Failed to load data');
      setLoading(false);
    } finally {
      isFetchingGlobal = false;
    }
  }, [adapter, deviceId, setLoading, setError]);

  // Initialize client and fetch data when adapter is available
  useEffect(() => {
    if (!adapter) {
      clientRef.current = null;
      return;
    }

    // Skip if already fetching or data already loaded
    if (isFetchingGlobal || patches.length > 0) {
      return;
    }

    fetchData();
  }, [adapter, patches.length, fetchData]);

  // Update patch names based on patchIndex loaded from function parameters
  useEffect(() => {
    if (patches.length === 0) return;

    // Look up patch names based on patchIndex from function parameters
    setParts((prev) =>
      prev.map((part) => {
        if (part.patchIndex !== null && part.patchIndex < patches.length) {
          const patch = patches[part.patchIndex];
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
    const patch = patchIndex !== null && patchIndex < patches.length ? patches[patchIndex] : null;
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
  const handleReload = async () => {
    if (!clientRef.current) return;

    // Invalidate cache and refetch
    clientRef.current.invalidatePatchCache();
    await fetchData();
  };

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
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-bold text-s330-text">Play</h2>
        <div className="flex gap-2">
          <button
            onClick={handleReload}
            disabled={isLoading}
            className={cn('btn btn-secondary', isLoading && 'opacity-50')}
          >
            {isLoading ? 'Loading...' : 'Refresh'}
          </button>
        </div>
      </div>

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
                  {patches.map((patch, patchIndex) => (
                    <option key={patchIndex} value={patchIndex}>
                      P{formatS330Number(patchIndex)} {isPatchEmpty(patch) ? '(empty)' : patch.common.name}
                    </option>
                  ))}
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

      {/* Error display */}
      {error && (
        <div className="bg-red-500/20 border border-red-500 rounded-md p-3">
          <p className="text-red-200 text-sm">{error}</p>
        </div>
      )}

      {/* Loading indicator */}
      {isLoading && (
        <div className="text-center py-4">
          <div className="animate-spin w-6 h-6 border-2 border-s330-highlight border-t-transparent rounded-full mx-auto" />
        </div>
      )}
    </div>
  );
}
