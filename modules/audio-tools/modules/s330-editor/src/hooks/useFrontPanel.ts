/**
 * React hook for S-330 Virtual Front Panel
 *
 * Provides button press functionality for remotely controlling
 * the S-330's front panel via SysEx messages.
 */

import { useState, useCallback, useMemo } from 'react';
import { useMidiStore } from '@/stores/midiStore';
import {
    createFrontPanelController,
    type FrontPanelButton,
    type NavigationButton,
    type FunctionButton,
    type FrontPanelController,
} from '@oletizi/sampler-devices/s330';

export interface UseFrontPanelState {
    /** Whether MIDI is connected */
    isConnected: boolean;
    /** Whether a button press is in progress */
    isPressing: boolean;
    /** Last error message, if any */
    lastError: string | null;
    /** The active button being pressed, if any */
    activeButton: FrontPanelButton | null;
}

export interface UseFrontPanelActions {
    /**
     * Press any front panel button
     */
    pressButton: (button: FrontPanelButton) => Promise<void>;

    /**
     * Press a navigation button
     */
    pressNavigation: (button: NavigationButton) => Promise<void>;

    /**
     * Press a function button
     */
    pressFunction: (button: FunctionButton) => Promise<void>;

    /**
     * Clear the last error
     */
    clearError: () => void;
}

export type UseFrontPanelReturn = UseFrontPanelState & UseFrontPanelActions;

/**
 * Hook for S-330 front panel remote control
 *
 * @example
 * ```tsx
 * function NavigationButtons() {
 *   const { pressButton, isPressing, isConnected } = useFrontPanel();
 *
 *   return (
 *     <button
 *       onClick={() => pressButton('up')}
 *       disabled={!isConnected || isPressing}
 *     >
 *       Up
 *     </button>
 *   );
 * }
 * ```
 */
export function useFrontPanel(): UseFrontPanelReturn {
    const [isPressing, setIsPressing] = useState(false);
    const [lastError, setLastError] = useState<string | null>(null);
    const [activeButton, setActiveButton] = useState<FrontPanelButton | null>(null);

    // Get MIDI state from store
    const adapter = useMidiStore((state) => state.adapter);
    const deviceId = useMidiStore((state) => state.deviceId);
    const status = useMidiStore((state) => state.status);

    const isConnected = status === 'connected' && adapter !== null;

    // Create controller when adapter is available
    const controller: FrontPanelController | null = useMemo(() => {
        if (!adapter) return null;
        return createFrontPanelController(adapter, { deviceId });
    }, [adapter, deviceId]);

    /**
     * Press any button (auto-detects type)
     */
    const pressButton = useCallback(async (button: FrontPanelButton): Promise<void> => {
        if (!controller) {
            setLastError('Not connected to MIDI device');
            return;
        }

        try {
            setIsPressing(true);
            setActiveButton(button);
            setLastError(null);
            await controller.press(button);
        } catch (err) {
            const message = err instanceof Error ? err.message : 'Failed to send button press';
            setLastError(message);
            console.error('[useFrontPanel] Button press failed:', message);
        } finally {
            setIsPressing(false);
            setActiveButton(null);
        }
    }, [controller]);

    /**
     * Press a navigation button
     */
    const pressNavigation = useCallback(async (button: NavigationButton): Promise<void> => {
        if (!controller) {
            setLastError('Not connected to MIDI device');
            return;
        }

        try {
            setIsPressing(true);
            setActiveButton(button);
            setLastError(null);
            await controller.pressNavigation(button);
        } catch (err) {
            const message = err instanceof Error ? err.message : 'Failed to send navigation button';
            setLastError(message);
            console.error('[useFrontPanel] Navigation button failed:', message);
        } finally {
            setIsPressing(false);
            setActiveButton(null);
        }
    }, [controller]);

    /**
     * Press a function button
     */
    const pressFunction = useCallback(async (button: FunctionButton): Promise<void> => {
        if (!controller) {
            setLastError('Not connected to MIDI device');
            return;
        }

        try {
            setIsPressing(true);
            setActiveButton(button);
            setLastError(null);
            await controller.pressFunction(button);
        } catch (err) {
            const message = err instanceof Error ? err.message : 'Failed to send function button';
            setLastError(message);
            console.error('[useFrontPanel] Function button failed:', message);
        } finally {
            setIsPressing(false);
            setActiveButton(null);
        }
    }, [controller]);

    /**
     * Clear the last error
     */
    const clearError = useCallback(() => {
        setLastError(null);
    }, []);

    return {
        isConnected,
        isPressing,
        lastError,
        activeButton,
        pressButton,
        pressNavigation,
        pressFunction,
        clearError,
    };
}

// Re-export button types for convenience
export type { FrontPanelButton, NavigationButton, FunctionButton };
