/**
 * Zustand store for S-330 UI state
 *
 * Data caching is handled by the S330 client - this store only manages
 * UI state like selection and loading indicators.
 */

import { create } from 'zustand';

interface S330State {
  // Selection state
  selectedPatchIndex: number | null;
  selectedToneIndex: number | null;

  // UI state
  isLoading: boolean;
  loadingMessage: string | null;
  error: string | null;
}

interface S330Actions {
  // Selection
  selectPatch: (index: number | null) => void;
  selectTone: (index: number | null) => void;

  // UI state
  setLoading: (isLoading: boolean, message?: string | null) => void;
  setError: (error: string | null) => void;
  clear: () => void;
}

type S330Store = S330State & S330Actions;

export const useS330Store = create<S330Store>((set) => ({
  // Initial state
  selectedPatchIndex: null,
  selectedToneIndex: null,
  isLoading: false,
  loadingMessage: null,
  error: null,

  selectPatch: (index) => set({ selectedPatchIndex: index }),

  selectTone: (index) => set({ selectedToneIndex: index }),

  setLoading: (isLoading, message = null) =>
    set({ isLoading, loadingMessage: message }),

  setError: (error) => set({ error }),

  clear: () =>
    set({
      selectedPatchIndex: null,
      selectedToneIndex: null,
      error: null,
    }),
}));
