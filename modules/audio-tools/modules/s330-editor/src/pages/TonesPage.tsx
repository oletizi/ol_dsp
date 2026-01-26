/**
 * Tones page - View and edit S-330 tones
 *
 * Parameter changes are sent to the device in real-time.
 * Data is cached in the S330 client - this page only manages UI state.
 * Loads first bank (16 tones) by default for faster startup.
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
  } = useS330Store();

  const isConnected = status === 'connected' && adapter !== null;

  // Keep a ref to the S330 client
  const clientRef = useRef<S330ClientInterface | null>(null);

  // Local state for tones (sparse array - undefined = not loaded)
  const [tones, setTones] = useState<(S330Tone | undefined)[]>([]);
  const [loadedBanks, setLoadedBanks] = useState<Set<number>>(new Set());

  // Initialize client when adapter changes
  useEffect(() => {
    if (!adapter) {
      clientRef.current = null;
      return;
    }
    clientRef.current = createS330Client(adapter, { deviceId });
  }, [adapter, deviceId]);

  // Load a specific bank of tones
  const loadToneBank = useCallback(async (bankIndex: number) => {
    if (!clientRef.current) return;

    const startIndex = bankIndex * TONES_PER_BANK;
    const count = TONES_PER_BANK;

    try {
      setLoading(true, `Loading tones ${startIndex + 1}-${startIndex + count}...`);
      setError(null);

      await clientRef.current.connect();
      const loadedTones = await clientRef.current.loadToneRange(startIndex, count, (current, total) => {
        setProgress(current, total);
      });

      // Update local state with loaded tones
      setTones((prev) => {
        const updated = [...prev];
        // Ensure array is large enough
        while (updated.length < TOTAL_TONES) updated.push(undefined);
        loadedTones.forEach((tone, i) => {
          updated[startIndex + i] = tone;
        });
        return updated;
      });
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

  // Load remaining bank
  const loadRemainingTones = useCallback(async () => {
    if (!loadedBanks.has(1)) {
      await loadToneBank(1);
    }
  }, [loadToneBank, loadedBanks]);

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

    clientRef.current.setTone(selectedToneIndex, toneData).catch((err) => {
      console.error('[TonesPage] Failed to send tone data:', err);
      setError(err instanceof Error ? err.message : 'Failed to send tone data');
    });
  }, [selectedToneIndex, tones, setError]);

  // Refresh: invalidate cache and reload first bank
  const handleRefresh = useCallback(async () => {
    if (!clientRef.current) return;

    clientRef.current.invalidateToneCache();
    setTones([]);
    setLoadedBanks(new Set());
    await loadInitialData();
  }, [loadInitialData]);

  // Auto-load initial data when connected
  useEffect(() => {
    if (isConnected && tones.length === 0 && !isLoading) {
      loadInitialData();
    }
  }, [isConnected, tones.length, isLoading, loadInitialData]);

  // Filter to only show loaded tones
  const loadedTonesArray = tones.filter((t): t is S330Tone => t !== undefined);
  const selectedTone = selectedToneIndex !== null ? tones[selectedToneIndex] : null;
  const allTonesLoaded = loadedBanks.has(0) && loadedBanks.has(1);

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
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-4">
          <h2 className="text-xl font-bold text-s330-text">Tones</h2>
          <span className="text-sm text-s330-muted">
            {loadedTonesArray.length} of {TOTAL_TONES} loaded
          </span>
        </div>
        <div className="flex gap-2">
          {!allTonesLoaded && (
            <button
              onClick={loadRemainingTones}
              disabled={isLoading}
              className={cn('btn btn-primary', isLoading && 'opacity-50')}
            >
              Load All Tones
            </button>
          )}
          <button
            onClick={handleRefresh}
            disabled={isLoading}
            className={cn('btn btn-secondary', isLoading && 'opacity-50')}
          >
            {isLoading ? 'Loading...' : 'Refresh'}
          </button>
        </div>
      </div>

      {/* Error Display */}
      {error && (
        <div className="bg-red-500/20 border border-red-500 rounded-md p-3">
          <p className="text-red-200 text-sm">{error}</p>
        </div>
      )}

      {/* Loading State */}
      {isLoading && (
        <div className="card text-center py-8">
          {loadingProgress !== null ? (
            <>
              <div className="w-full max-w-md mx-auto mb-4">
                <div className="h-2 bg-s330-bg rounded-full overflow-hidden">
                  <div
                    className="h-full bg-s330-highlight transition-all duration-150 ease-out"
                    style={{ width: `${loadingProgress}%` }}
                  />
                </div>
              </div>
              <p className="text-s330-muted">
                {loadingMessage}
              </p>
            </>
          ) : (
            <>
              <div className="animate-spin w-8 h-8 border-2 border-s330-highlight border-t-transparent rounded-full mx-auto mb-4" />
              <p className="text-s330-muted">{loadingMessage ?? 'Loading...'}</p>
            </>
          )}
        </div>
      )}

      {/* Content */}
      {!isLoading && loadedTonesArray.length > 0 && (
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
