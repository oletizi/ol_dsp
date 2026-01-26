/**
 * Tests for S-330 Front Panel Controller
 *
 * Tests the virtual front panel button message encoding.
 * Arrow buttons use category 01 codes (single message).
 * Inc/Dec buttons use category 09 codes (press + delay + release).
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest';
import {
    createFrontPanelController,
    buildFrontPanelMessage,
    isNavigationButton,
    isArrowButton,
    isValueButton,
    FRONT_PANEL_ADDRESS,
    ARROW_CODES,
    VALUE_CODES,
    VALUE_BUTTON_DELAY_MS,
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

    describe('isArrowButton', () => {
        it('should identify arrow buttons', () => {
            expect(isArrowButton('up')).toBe(true);
            expect(isArrowButton('down')).toBe(true);
            expect(isArrowButton('left')).toBe(true);
            expect(isArrowButton('right')).toBe(true);
        });

        it('should reject inc/dec buttons', () => {
            expect(isArrowButton('inc')).toBe(false);
            expect(isArrowButton('dec')).toBe(false);
        });
    });

    describe('isValueButton', () => {
        it('should identify value buttons', () => {
            expect(isValueButton('inc')).toBe(true);
            expect(isValueButton('dec')).toBe(true);
        });

        it('should reject arrow buttons', () => {
            expect(isValueButton('up')).toBe(false);
            expect(isValueButton('down')).toBe(false);
            expect(isValueButton('left')).toBe(false);
            expect(isValueButton('right')).toBe(false);
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

        it('should calculate correct checksum for arrow button', () => {
            // Address: 00 04 00 00, Data: 01 02 (up)
            // Sum = 0 + 4 + 0 + 0 + 1 + 2 = 7
            // Checksum = 128 - 7 = 121 = 0x79
            const message = buildFrontPanelMessage(0, [0x01, 0x02]);
            expect(message[11]).toBe(0x79);
        });

        it('should calculate correct checksum for value button press', () => {
            // Address: 00 04 00 00, Data: 09 04 (inc press)
            // Sum = 0 + 4 + 0 + 0 + 9 + 4 = 17
            // Checksum = 128 - 17 = 111 = 0x6F
            const message = buildFrontPanelMessage(0, [0x09, 0x04]);
            expect(message[11]).toBe(0x6f);
        });

        it('should calculate correct checksum for function button', () => {
            // Address: 00 04 00 00, Data: 01 0C (menu)
            // Sum = 0 + 4 + 0 + 0 + 1 + 12 = 17
            // Checksum = 128 - 17 = 111 = 0x6F
            const message = buildFrontPanelMessage(0, [0x01, 0x0c]);
            expect(message[11]).toBe(0x6f);
        });
    });

    describe('ARROW_CODES', () => {
        it('should have correct category 01 codes for all arrow buttons', () => {
            expect(ARROW_CODES.right).toEqual([0x01, 0x00]);
            expect(ARROW_CODES.left).toEqual([0x01, 0x01]);
            expect(ARROW_CODES.up).toEqual([0x01, 0x02]);
            expect(ARROW_CODES.down).toEqual([0x01, 0x03]);
        });

        it('should all use category 01', () => {
            for (const [_button, code] of Object.entries(ARROW_CODES)) {
                expect(code[0]).toBe(0x01);
            }
        });
    });

    describe('VALUE_CODES', () => {
        it('should have correct category 09 codes for inc/dec', () => {
            expect(VALUE_CODES.inc.press).toEqual([0x09, 0x04]);
            expect(VALUE_CODES.inc.release).toEqual([0x09, 0x0c]);
            expect(VALUE_CODES.dec.press).toEqual([0x09, 0x05]);
            expect(VALUE_CODES.dec.release).toEqual([0x09, 0x0d]);
        });

        it('should have release codes that are press codes + 8', () => {
            expect(VALUE_CODES.inc.release[1]).toBe(VALUE_CODES.inc.press[1] + 8);
            expect(VALUE_CODES.dec.release[1]).toBe(VALUE_CODES.dec.press[1] + 8);
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
            vi.useFakeTimers();
        });

        afterEach(() => {
            vi.useRealTimers();
        });

        describe('pressNavigation - arrow buttons', () => {
            const arrowButtons: Array<'up' | 'down' | 'left' | 'right'> = ['up', 'down', 'left', 'right'];

            it.each(arrowButtons)('should send single message for %s button', async (button) => {
                const controller = createFrontPanelController(adapter, { deviceId: 0 });
                await controller.pressNavigation(button);

                // Should have sent only one message
                expect(adapter.sentMessages).toHaveLength(1);

                // Verify message uses category 01
                const code = ARROW_CODES[button];
                verifyMessageStructure(adapter.sentMessages[0], 0, code as [number, number]);
            });

            it('should use specified device ID', async () => {
                const controller = createFrontPanelController(adapter, { deviceId: 7 });
                await controller.pressNavigation('up');

                expect(adapter.sentMessages[0][2]).toBe(7);
            });
        });

        describe('pressNavigation - value buttons (inc/dec)', () => {
            const valueButtons: Array<'inc' | 'dec'> = ['inc', 'dec'];

            it.each(valueButtons)('should send press and release for %s button', async (button) => {
                const controller = createFrontPanelController(adapter, { deviceId: 0 });
                const promise = controller.pressNavigation(button);

                // Advance past the delay
                await vi.advanceTimersByTimeAsync(VALUE_BUTTON_DELAY_MS);
                await promise;

                // Should have sent two messages: press and release
                expect(adapter.sentMessages).toHaveLength(2);

                // Verify press message
                const codes = VALUE_CODES[button];
                verifyMessageStructure(adapter.sentMessages[0], 0, codes.press as [number, number]);

                // Verify release message
                verifyMessageStructure(adapter.sentMessages[1], 0, codes.release as [number, number]);
            });

            it('should use specified device ID for inc/dec', async () => {
                const controller = createFrontPanelController(adapter, { deviceId: 9 });
                const promise = controller.pressNavigation('inc');

                await vi.advanceTimersByTimeAsync(VALUE_BUTTON_DELAY_MS);
                await promise;

                expect(adapter.sentMessages[0][2]).toBe(9);
                expect(adapter.sentMessages[1][2]).toBe(9);
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
            it('should handle arrow buttons with single message', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('left');

                expect(adapter.sentMessages).toHaveLength(1);
                expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
            });

            it('should handle inc/dec with press and release', async () => {
                const controller = createFrontPanelController(adapter);
                const promise = controller.press('inc');

                await vi.advanceTimersByTimeAsync(VALUE_BUTTON_DELAY_MS);
                await promise;

                expect(adapter.sentMessages).toHaveLength(2);
                expect(adapter.sentMessages[0][9]).toBe(0x09); // Category 09 press
                expect(adapter.sentMessages[1][9]).toBe(0x09); // Category 09 release
            });

            it('should handle function buttons with single message', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('execute');

                expect(adapter.sentMessages).toHaveLength(1);
                expect(adapter.sentMessages[0][9]).toBe(0x01); // Category 01
            });

            it('should use default device ID when not specified', async () => {
                const controller = createFrontPanelController(adapter);
                await controller.press('up');

                expect(adapter.sentMessages[0][2]).toBe(0);
            });
        });
    });

    describe('message checksums', () => {
        it('should produce valid checksums for all arrow codes', () => {
            for (const [_button, code] of Object.entries(ARROW_CODES)) {
                const message = buildFrontPanelMessage(0, code as [number, number]);

                // Verify by recalculating
                const expectedChecksum = calculateChecksum(FRONT_PANEL_ADDRESS, code as [number, number]);
                expect(message[11]).toBe(expectedChecksum);
            }
        });

        it('should produce valid checksums for all value button press codes', () => {
            for (const [_button, codes] of Object.entries(VALUE_CODES)) {
                const message = buildFrontPanelMessage(0, codes.press as [number, number]);

                // Verify by recalculating
                const expectedChecksum = calculateChecksum(FRONT_PANEL_ADDRESS, codes.press as [number, number]);
                expect(message[11]).toBe(expectedChecksum);
            }
        });

        it('should produce valid checksums for all value button release codes', () => {
            for (const [_button, codes] of Object.entries(VALUE_CODES)) {
                const message = buildFrontPanelMessage(0, codes.release as [number, number]);

                // Verify by recalculating
                const expectedChecksum = calculateChecksum(FRONT_PANEL_ADDRESS, codes.release as [number, number]);
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
