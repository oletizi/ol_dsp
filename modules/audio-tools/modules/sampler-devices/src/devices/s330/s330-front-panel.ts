/**
 * S-330 Virtual Front Panel Controller
 *
 * Sends SysEx messages to remotely control the S-330's front panel buttons.
 * Button events are sent as DT1 messages to address 00 04 00 00.
 *
 * Message format: F0 41 [dev] 1E 12 00 04 00 00 [cat] [code] [checksum] F7
 *
 * All buttons use category 01 codes which work in all contexts
 * (both popup menus and parameter editing screens).
 *
 * @packageDocumentation
 */

import type { S330MidiAdapter } from './s330-types.js';
import {
    ROLAND_ID,
    S330_MODEL_ID,
    S330_COMMANDS,
    calculateChecksum,
} from './s330-addresses.js';

// =============================================================================
// Types
// =============================================================================

/**
 * Navigation buttons (arrows and inc/dec)
 */
export type NavigationButton = 'up' | 'down' | 'left' | 'right' | 'inc' | 'dec';

/**
 * Function buttons
 */
export type FunctionButton = 'mode' | 'menu' | 'sub-menu' | 'com' | 'execute';

/**
 * All available front panel buttons
 */
export type FrontPanelButton = NavigationButton | FunctionButton;

/**
 * Options for creating the front panel controller
 */
export interface FrontPanelControllerOptions {
    /** Device ID (0-16), defaults to 0 */
    deviceId?: number;
}

/**
 * Front panel controller interface
 */
export interface FrontPanelController {
    /**
     * Press a navigation button (single category 01 message)
     */
    pressNavigation(button: NavigationButton): Promise<void>;

    /**
     * Press a function button (single category 01 message)
     */
    pressFunction(button: FunctionButton): Promise<void>;

    /**
     * Press any front panel button (auto-detects type)
     */
    press(button: FrontPanelButton): Promise<void>;
}

// =============================================================================
// Constants
// =============================================================================

/** Front panel button event address */
export const FRONT_PANEL_ADDRESS = [0x00, 0x04, 0x00, 0x00];

/**
 * Category 01 - Arrow button codes (single message)
 *
 * These codes work in all contexts (menus and parameter screens).
 */
export const ARROW_CODES = {
    right: [0x01, 0x00],
    left: [0x01, 0x01],
    up: [0x01, 0x02],
    down: [0x01, 0x03],
} as const;

/**
 * Category 09 - Inc/Dec button codes (press + delay + release)
 *
 * Inc/Dec require press/release pairs with category 09.
 */
export const VALUE_CODES = {
    inc: { press: [0x09, 0x04], release: [0x09, 0x0c] },
    dec: { press: [0x09, 0x05], release: [0x09, 0x0d] },
} as const;

/** Default delay between press and release for inc/dec buttons */
export const VALUE_BUTTON_DELAY_MS = 50;

/**
 * Category 01 - Function button codes (single message)
 */
export const FUNCTION_CODES = {
    mode: [0x01, 0x0b],
    menu: [0x01, 0x0c],
    'sub-menu': [0x01, 0x0d],
    com: [0x01, 0x0e],
    execute: [0x01, 0x0f],
} as const;

// =============================================================================
// Helpers
// =============================================================================

/**
 * Set of navigation button names for type checking
 */
const NAVIGATION_BUTTONS: Set<string> = new Set(['up', 'down', 'left', 'right', 'inc', 'dec']);

/**
 * Set of arrow button names
 */
const ARROW_BUTTONS: Set<string> = new Set(['up', 'down', 'left', 'right']);

/**
 * Set of value button names (inc/dec)
 */
const VALUE_BUTTONS: Set<string> = new Set(['inc', 'dec']);

/**
 * Check if a button is a navigation button
 */
export function isNavigationButton(button: FrontPanelButton): button is NavigationButton {
    return NAVIGATION_BUTTONS.has(button);
}

/**
 * Check if a button is an arrow button
 */
export function isArrowButton(button: NavigationButton): button is 'up' | 'down' | 'left' | 'right' {
    return ARROW_BUTTONS.has(button);
}

/**
 * Check if a button is a value button (inc/dec)
 */
export function isValueButton(button: NavigationButton): button is 'inc' | 'dec' {
    return VALUE_BUTTONS.has(button);
}

/**
 * Delay helper
 */
function delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

/**
 * Build a front panel DT1 message
 *
 * @param deviceId - Device ID (0-16)
 * @param data - Two-byte data: [category, code]
 * @returns Complete SysEx message array
 */
export function buildFrontPanelMessage(deviceId: number, data: [number, number]): number[] {
    const checksum = calculateChecksum(FRONT_PANEL_ADDRESS, data);

    return [
        0xf0,
        ROLAND_ID,
        deviceId,
        S330_MODEL_ID,
        S330_COMMANDS.DT1,
        ...FRONT_PANEL_ADDRESS,
        ...data,
        checksum,
        0xf7,
    ];
}

// =============================================================================
// Factory
// =============================================================================

/**
 * Create a front panel controller for sending button events to the S-330
 *
 * @param midiAdapter - MIDI adapter for sending messages
 * @param options - Controller options
 * @returns FrontPanelController instance
 *
 * @example
 * ```typescript
 * const controller = createFrontPanelController(adapter, { deviceId: 0 });
 *
 * // Navigate
 * await controller.pressNavigation('right');
 * await controller.pressNavigation('inc');
 *
 * // Open menu
 * await controller.pressFunction('menu');
 *
 * // Or use the unified press method
 * await controller.press('up');
 * await controller.press('execute');
 * ```
 */
export function createFrontPanelController(
    midiAdapter: S330MidiAdapter,
    options: FrontPanelControllerOptions = {}
): FrontPanelController {
    const deviceId = options.deviceId ?? 0;

    return {
        async pressNavigation(button: NavigationButton): Promise<void> {
            if (isArrowButton(button)) {
                // Arrow buttons use category 01 single message
                const code = ARROW_CODES[button];
                const message = buildFrontPanelMessage(deviceId, code as [number, number]);
                midiAdapter.send(message);
            } else if (isValueButton(button)) {
                // Inc/Dec use category 09 press + delay + release
                const codes = VALUE_CODES[button];
                const pressMessage = buildFrontPanelMessage(deviceId, codes.press as [number, number]);
                const releaseMessage = buildFrontPanelMessage(deviceId, codes.release as [number, number]);
                midiAdapter.send(pressMessage);
                await delay(VALUE_BUTTON_DELAY_MS);
                midiAdapter.send(releaseMessage);
            }
        },

        async pressFunction(button: FunctionButton): Promise<void> {
            const code = FUNCTION_CODES[button];
            const message = buildFrontPanelMessage(deviceId, code as [number, number]);
            midiAdapter.send(message);
        },

        async press(button: FrontPanelButton): Promise<void> {
            if (isNavigationButton(button)) {
                await this.pressNavigation(button);
            } else {
                await this.pressFunction(button);
            }
        },
    };
}
