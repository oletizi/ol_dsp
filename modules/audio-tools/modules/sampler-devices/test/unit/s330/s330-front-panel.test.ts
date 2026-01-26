/**
 * Tests for S-330 Front Panel Controller
 *
 * Tests the virtual front panel button message encoding.
 * All buttons use category 01 codes which work in all contexts.
 */

import { describe, it, expect, beforeEach } from 'vitest';
import {
    createFrontPanelController,
    buildFrontPanelMessage,
    isNavigationButton,
    FRONT_PANEL_ADDRESS,
    NAVIGATION_CODES,
    FUNCTION_CODES,
    type NavigationButton,
    type FunctionButton,
    type FrontPanelButton,
} from '../../../src/devices/s330/s330-front-panel.js';
import type { S330MidiAdapter } from '../../../src/devices/s330/s330-types.js';
import { calculateChecksum } from '../../../src/devices/s330/s330-addresses.js';

// Constants
const ROLAND_ID = 0x41;
const S330_MODEL_ID = 0x1e;
const DT1_COMMAND = 0x12;

/**
 * Create a mock MIDI adapter
 */
function createMockAdapter(): S330MidiAdapter & { sentMessages: number[][] } {
    const sentMessages: number[][] = [];
    return {
        sentMessages,
        send(data: number[]) {
            sentMessages.push([...data]);
        },
        onSysEx() {},
        removeSysExListener() {},
    };
}

/**
 * Verify a front panel message has correct structure
 */
function verifyMessageStructure(
    message: number[],
    deviceId: number,
    expectedData: [number, number]
): void {
    expect(message[0]).toBe(0xf0); // SysEx start
    expect(message[1]).toBe(ROLAND_ID);
    expect(message[2]).toBe(deviceId);
    expect(message[3]).toBe(S330_MODEL_ID);
    expect(message[4]).toBe(DT1_COMMAND);
    expect(message.slice(5, 9)).toEqual(FRONT_PANEL_ADDRESS);
    expect(message[9]).toBe(expectedData[0]); // Category
    expect(message[10]).toBe(expectedData[1]); // Code
    expect(message[message.length - 1]).toBe(0xf7); // SysEx end

    // Verify checksum
    const checksum = calculateChecksum(FRONT_PANEL_ADDRESS, expectedData);
    expect(message[11]).toBe(checksum);
}

describe('s330-front-panel', () => {
    describe('isNavigationButton', () => {
        it('should identify navigation buttons', () => {
            expect(isNavigationButton('up')).toBe(true);
            expect(isNavigationButton('down')).toBe(true);
            expect(isNavigationButton('left')).toBe(true);
            expect(isNavigationButton('right')).toBe(true);
            expect(isNavigationButton('inc')).toBe(true);
            expect(isNavigationButton('dec')).toBe(true);
        });

        it('should reject function buttons', () => {
            expect(isNavigationButton('mode' as FrontPanelButton)).toBe(false);
            expect(isNavigationButton('menu' as FrontPanelButton)).toBe(false);
            expect(isNavigationButton('sub-menu' as FrontPanelButton)).toBe(false);
            expect(isNavigationButton('com' as FrontPanelButton)).toBe(false);
            expect(isNavigationButton('execute' as FrontPanelButton)).toBe(false);
        });
    });

    describe('buildFrontPanelMessage', () => {
        it('should build correct message structure', () => {
            const message = buildFrontPanelMessage(0, [0x01, 0x00]);
            verifyMessageStructure(message, 0, [0x01, 0x00]);
        });

        it('should respect device ID', () => {
            const message = buildFrontPanelMessage(5, [0x01, 0x0c]);
            expect(message[2]).toBe(5);
        });

        it('should calculate correct checksum for navigation button', () => {
            // Address: 00 04 00 00, Data: 01 04 (inc)
            // Sum = 0 + 4 + 0 + 0 + 1 + 4 = 9
            // Checksum = 128 - 9 = 119 = 0x77
            const message = buildFrontPanelMessage(0, [0x01, 0x04]);
            expect(message[11]).toBe(0x77);
        });

        it('should calculate correct checksum for function button', () => {
            // Address: 00 04 00 00, Data: 01 0C (menu)
            // Sum = 0 + 4 + 0 + 0 + 1 + 12 = 17
            // Checksum = 128 - 17 = 111 = 0x6F
            const message = buildFrontPanelMessage(0, [0x01, 0x0c]);
            expect(message[11]).toBe(0x6f);
        });
    });

    describe('NAVIGATION_CODES', () => {
        it('should have correct category 01 codes for all navigation buttons', () => {
            expect(NAVIGATION_CODES.right).toEqual([0x01, 0x00]);
            expect(NAVIGATION_CODES.left).toEqual([0x01, 0x01]);
            expect(NAVIGATION_CODES.up).toEqual([0x01, 0x02]);
            expect(NAVIGATION_CODES.down).toEqual([0x01, 0x03]);
            expect(NAVIGATION_CODES.inc).toEqual([0x01, 0x04]);
            expect(NAVIGATION_CODES.dec).toEqual([0x01, 0x05]);
        });

        it('should all use category 01', () => {
            for (const [_button, code] of Object.entries(NAVIGATION_CODES)) {
                expect(code[0]).toBe(0x01);
            }
        });
    });

    describe('FUNCTION_CODES', () => {
        it('should have correct codes for all function buttons', () => {
            expect(FUNCTION_CODES.mode).toEqual([0x01, 0x0b]);
            expect(FUNCTION_CODES.menu).toEqual([0x01, 0x0c]);
            expect(FUNCTION_CODES['sub-menu']).toEqual([0x01, 0x0d]);
            expect(FUNCTION_CODES.com).toEqual([0x01, 0x0e]);
            expect(FUNCTION_CODES.execute).toEqual([0x01, 0x0f]);
        });

        it('should all use category 01', () => {
            for (const [_button, code] of Object.entries(FUNCTION_CODES)) {
                expect(code[0]).toBe(0x01);
            }
        });
    });

    describe('createFrontPanelController', () => {
        let adapter: ReturnType<typeof createMockAdapter>;

        beforeEach(() => {
            adapter = createMockAdapter();
        });

        describe('pressNavigation', () => {
            const navigationButtons: NavigationButton[] = ['up', 'down', 'left', 'right', 'inc', 'dec'];

            it.each(navigationButtons)('should send single message for %s button', async (button) => {
                const controller = createFrontPanelController(adapter, { deviceId: 0 });
                await controller.pressNavigation(button);

                // Should have sent only one message
                expect(adapter.sentMessages).toHaveLength(1);

                // Verify message
                const code = NAVIGATION_CODES[button];
                verifyMessageStructure(adapter.sentMessages[0], 0, code as [number, number]);
            });

            it('should use specified device ID', async () => {
                const controller = createFrontPanelController(adapter, { deviceId: 7 });
                await controller.pressNavigation('inc');

                expect(adapter.sentMessages[0][2]).toBe(7);
            });
        });

        describe('pressFunction', () => {
            const functionButtons: FunctionButton[] = ['mode', 'menu', 'sub-menu', 'com', 'execute'];

            it.each(functionButtons)('should send single message for %s button', async (button) => {
                const controller = createFrontPanelController(adapter, { deviceId: 0 });
                await controller.pressFunction(button);

                // Should have sent only one message
                expect(adapter.sentMessages).toHaveLength(1);

                // Verify message
                const code = FUNCTION_CODES[button];
                verifyMessageStructure(adapter.sentMessages[0], 0, code as [number, number]);
            });

            it('should use specified device ID', async () => {
                const controller = createFrontPanelController(adapter, { deviceId: 12 });
                await controller.pressFunction('menu');

                expect(adapter.sentMessages[0][2]).toBe(12);
            });
        });

        describe('press (unified)', () => {
            it('should handle navigation buttons', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('left');

                expect(adapter.sentMessages).toHaveLength(1);
                expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
            });

            it('should handle function buttons', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('execute');

                expect(adapter.sentMessages).toHaveLength(1);
                expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
            });

            it('should work with all navigation buttons', async () => {
                const controller = createFrontPanelController(adapter);

                for (const button of ['up', 'down', 'left', 'right', 'inc', 'dec'] as NavigationButton[]) {
                    adapter.sentMessages.length = 0;
                    await controller.press(button);
                    expect(adapter.sentMessages).toHaveLength(1);
                    expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
                }
            });

            it('should work with all function buttons', async () => {
                const controller = createFrontPanelController(adapter);

                for (const button of ['mode', 'menu', 'sub-menu', 'com', 'execute'] as FunctionButton[]) {
                    adapter.sentMessages.length = 0;
                    await controller.press(button);
                    expect(adapter.sentMessages).toHaveLength(1);
                    expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
                }
            });

            it('should use default device ID when not specified', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('up');

                expect(adapter.sentMessages[0][2]).toBe(0);
            });
        });
    });

    describe('message checksums', () => {
        it('should produce valid checksums for all navigation codes', () => {
            for (const [_button, code] of Object.entries(NAVIGATION_CODES)) {
                const message = buildFrontPanelMessage(0, code as [number, number]);

                // Verify by recalculating
                const expectedChecksum = calculateChecksum(FRONT_PANEL_ADDRESS, code as [number, number]);
                expect(message[11]).toBe(expectedChecksum);
            }
        });

        it('should produce valid checksums for all function codes', () => {
            for (const [_button, code] of Object.entries(FUNCTION_CODES)) {
                const message = buildFrontPanelMessage(0, code as [number, number]);

                // Verify by recalculating
                const expectedChecksum = calculateChecksum(FRONT_PANEL_ADDRESS, code as [number, number]);
                expect(message[11]).toBe(expectedChecksum);
            }
        });
    });
});
