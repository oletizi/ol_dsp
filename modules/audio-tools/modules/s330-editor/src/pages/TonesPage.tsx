/**
 * Tones page - View and edit S-330 tones
 *
 * Parameter changes are sent to the device in real-time.
 * Data is cached in the S330 client - this page only manages UI state.
 * Loads first bank (8 tones) by default for faster startup.
 */

import { useCallback, useEffect, useRef, useState } from 'react';
import { useMidiStore } from '@/stores/midiStore';
import { useS330Store } from '@/stores/s330Store';
import { createS330Client } from '@/core/midi/S330Client';
import type { S330ClientInterface, S330Tone } from '@/core/midi/S330Client';
import { ToneList } from '@/components/tones/ToneList';
import { ToneEditor } from '@/components/tones/ToneEditor';
import { cn } from '@/lib/utils';

// Constants for bank loading (S-330 uses banks of 8)
const TONES_PER_BANK = 8;
const TOTAL_TONES = 32;

export function TonesPage() {
  const { adapter, deviceId, status } = useMidiStore();
  const {
    selectedToneIndex,
    isLoading,
    loadingMessage,
    loadingProgress,
    error,
    selectTone,
    setLoading,
    setError,
    setProgress,
    clearProgress,
    hardwareChangeVersion,
    lastHardwareChange,
  } = useS330Store();

  const isConnected = status === 'connected' && adapter !== null;

  // Keep a ref to the S330 client
  const clientRef = useRef<S330ClientInterface | null>(null);

  // Local state for tones (sparse array - undefined = not loaded)
  const [tones, setTones] = useState<(S330Tone | undefined)[]>([]);
  const [loadedBanks, setLoadedBanks] = useState<Set<number>>(new Set());

  // Track last processed hardware change to prevent refetch loops
  const lastProcessedChangeRef = useRef<number>(0);

  // Debounce timer for hardware change refetch
  // Delay refetch until user releases button to avoid feedback loop
  const refetchTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const REFETCH_DEBOUNCE_MS = 500; // Wait 500ms after last change before refetch

  // Get notifyHardwareChange from store
  const notifyHardwareChange = useS330Store((state) => state.notifyHardwareChange);

  // Initialize client when adapter changes
  useEffect(() => {
    if (!adapter) {
      clientRef.current = null;
      return;
    }
    const client = createS330Client(adapter, { deviceId });
    clientRef.current = client;

    // Start listening for hardware parameter changes
    client.startListening();
    client.onParameterChange((event) => {
      console.log('[TonesPage] Hardware parameter change:', event.type, 'index:', event.index);
      notifyHardwareChange(event.type, event.index);
    });

    return () => {
      client.stopListening();
    };
  }, [adapter, deviceId, notifyHardwareChange]);

  // Sync loaded state from client cache on mount
  useEffect(() => {
    if (!clientRef.current) return;

    const cachedTones = clientRef.current.getLoadedTones();
    const banksWithData = new Set<number>();

    // Check each bank to see if it has cached data
    for (let bank = 0; bank < 4; bank++) {
      const startIndex = bank * TONES_PER_BANK;
      const hasData = cachedTones.slice(startIndex, startIndex + TONES_PER_BANK).some(t => t !== undefined);
      if (hasData) {
        banksWithData.add(bank);
      }
    }

    if (banksWithData.size > 0) {
      setLoadedBanks(banksWithData);
      setTones(cachedTones);
    }
  }, [adapter, deviceId]);

  // Load a specific bank of tones (updates UI progressively)
  const loadToneBank = useCallback(async (bankIndex: number, forceReload = false) => {
    if (!clientRef.current) return;

    const startIndex = bankIndex * TONES_PER_BANK;
    const count = TONES_PER_BANK;

    try {
      setLoading(true, `${forceReload ? 'Reloading' : 'Loading'} tones ${startIndex + 1}-${startIndex + count}...`);
      setError(null);

      // Ensure array is large enough before loading
      setTones((prev) => {
        if (prev.length >= TOTAL_TONES) return prev;
        const updated = [...prev];
        while (updated.length < TOTAL_TONES) updated.push(undefined);
        return updated;
      });

      await clientRef.current.connect();
      await clientRef.current.loadToneRange(
        startIndex,
        count,
        (current, total) => setProgress(current, total),
        // Update UI immediately when each tone is loaded
        (index, tone) => {
          setTones((prev) => {
            const updated = [...prev];
            updated[index] = tone;
            return updated;
          });
        },
        forceReload
      );

      setLoadedBanks((prev) => new Set([...prev, bankIndex]));
      clearProgress();
      setLoading(false);
    } catch (err) {
      const message = err instanceof Error ? err.message : 'Failed to load tones';
      setError(message);
      clearProgress();
      setLoading(false);
    }
  }, [setLoading, setError, setProgress, clearProgress]);

  // Load initial data (first bank)
  const loadInitialData = useCallback(async () => {
    await loadToneBank(0);
  }, [loadToneBank]);

  // Handle tone parameter updates - local state update for responsive UI
  const handleToneUpdate = useCallback((updatedTone: S330Tone) => {
    if (selectedToneIndex === null) return;
    setTones((prev) => {
      const updated = [...prev];
      updated[selectedToneIndex] = updatedTone;
      return updated;
    });
  }, [selectedToneIndex]);

  // Commit changes to device via client (updates cache + hardware)
  const handleToneCommit = useCallback((updatedTone?: S330Tone) => {
    if (selectedToneIndex === null || !clientRef.current) return;

    // Use provided tone or fall back to local state
    const toneData = updatedTone ?? tones[selectedToneIndex];
    if (!toneData) return;

    console.log(`[TonesPage] handleToneCommit called for tone ${selectedToneIndex}`);

    clientRef.current.setTone(selectedToneIndex, toneData).catch((err) => {
      console.error('[TonesPage] Failed to send tone data:', err);
      setError(err instanceof Error ? err.message : 'Failed to send tone data');
    });
  }, [selectedToneIndex, tones, setError]);

  // Refresh: invalidate cache and reload first bank
  // Load all tone banks
  const loadAllTones = useCallback(async () => {
    if (!clientRef.current) return;

    clientRef.current.invalidateToneCache();
    setTones([]);
    setLoadedBanks(new Set());

    // Load all 4 tone banks sequentially
    for (let bank = 0; bank < 4; bank++) {
      await loadToneBank(bank, true);
    }
  }, [loadToneBank]);

  // Auto-load initial data when connected
  useEffect(() => {
    if (isConnected && tones.length === 0 && !isLoading) {
      loadInitialData();
    }
  }, [isConnected, tones.length, isLoading, loadInitialData]);

  // Handle hardware parameter changes with debounced refetch
  // Wait for user to release button before refetching to avoid feedback loop
  useEffect(() => {
    // Skip if we've already processed this change
    if (hardwareChangeVersion <= lastProcessedChangeRef.current) {
      return;
    }

    // Skip if no change or not a tone change
    if (lastHardwareChange.type !== 'tone' || lastHardwareChange.index === null) {
      return;
    }

    const toneIndex = lastHardwareChange.index;

    // Mark this change as processed
    lastProcessedChangeRef.current = hardwareChangeVersion;

    console.log(`[TonesPage] Hardware changed tone ${toneIndex} - scheduling debounced refetch`);

    // Clear any pending refetch
    if (refetchTimerRef.current) {
      clearTimeout(refetchTimerRef.current);
    }

    // Schedule debounced refetch - wait for user to release button
    refetchTimerRef.current = setTimeout(async () => {
      if (!clientRef.current) return;

      console.log(`[TonesPage] Refetching tone ${toneIndex} after debounce`);
      try {
        // Force reload just this one tone
        await clientRef.current.loadToneRange(toneIndex, 1, undefined, (index, tone) => {
          setTones((prev) => {
            const updated = [...prev];
            updated[index] = tone;
            return updated;
          });
        }, true);
      } catch (err) {
        console.error(`[TonesPage] Failed to refetch tone ${toneIndex}:`, err);
      }
    }, REFETCH_DEBOUNCE_MS);
  }, [hardwareChangeVersion, lastHardwareChange]);

  // Cleanup refetch timer on unmount
  useEffect(() => {
    return () => {
      if (refetchTimerRef.current) {
        clearTimeout(refetchTimerRef.current);
      }
    };
  }, []);

  // Filter to only show loaded tones
  const loadedTonesArray = tones.filter((t): t is S330Tone => t !== undefined);
  const selectedTone = selectedToneIndex !== null ? tones[selectedToneIndex] : null;

  if (!isConnected) {
    return (
      <div className="card text-center py-12">
        <h2 className="text-xl font-bold text-s330-text mb-2">Not Connected</h2>
        <p className="text-s330-muted mb-4">
          Connect to your S-330 to view and edit tones.
        </p>
        <a href="/" className="btn btn-primary inline-block">
          Go to Connection
        </a>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between gap-4">
        <div className="flex items-center gap-4">
          <h2 className="text-xl font-bold text-s330-text">Tones</h2>
          <span className="text-sm text-s330-muted">
            {loadedTonesArray.length} of {TOTAL_TONES} loaded
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
          <div className="flex items-center gap-2">
            <span className="text-sm text-s330-muted">(Re)load:</span>
            <button
              onClick={() => loadToneBank(0, true)}
              disabled={isLoading}
              className={cn('btn', loadedBanks.has(0) ? 'btn-secondary' : 'btn-primary', isLoading && 'opacity-50')}
            >
              T11-T18
            </button>
            <button
              onClick={() => loadToneBank(1, true)}
              disabled={isLoading}
              className={cn('btn', loadedBanks.has(1) ? 'btn-secondary' : 'btn-primary', isLoading && 'opacity-50')}
            >
              T21-T28
            </button>
            <button
              onClick={() => loadToneBank(2, true)}
              disabled={isLoading}
              className={cn('btn', loadedBanks.has(2) ? 'btn-secondary' : 'btn-primary', isLoading && 'opacity-50')}
            >
              T31-T38
            </button>
            <button
              onClick={() => loadToneBank(3, true)}
              disabled={isLoading}
              className={cn('btn', loadedBanks.has(3) ? 'btn-secondary' : 'btn-primary', isLoading && 'opacity-50')}
            >
              T41-T48
            </button>
            <button
              onClick={loadAllTones}
              disabled={isLoading}
              className={cn('btn btn-secondary', isLoading && 'opacity-50')}
            >
              All
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

      {/* Content - show while loading for progressive updates */}
      {tones.length > 0 && (
        <div className="grid gap-6 lg:grid-cols-3">
          <div className="lg:col-span-1">
            <ToneList
              tones={tones}
              selectedIndex={selectedToneIndex}
              onSelect={selectTone}
            />
          </div>
          <div className="lg:col-span-2">
            {selectedTone ? (
              <ToneEditor
                tone={selectedTone}
                index={selectedToneIndex!}
                onUpdate={handleToneUpdate}
                onCommit={handleToneCommit}
              />
            ) : (
              <div className="card text-center py-12 text-s330-muted">
                Select a tone to edit
              </div>
            )}
          </div>
        </div>
      )}

      {/* Empty State - no tones loaded yet */}
      {!isLoading && loadedTonesArray.length === 0 && !error && (
        <div className="card text-center py-12">
          <p className="text-s330-muted mb-4">No tones loaded</p>
          <button onClick={loadInitialData} className="btn btn-primary">
            Load Tones
          </button>
        </div>
      )}
    </div>
  );
}
