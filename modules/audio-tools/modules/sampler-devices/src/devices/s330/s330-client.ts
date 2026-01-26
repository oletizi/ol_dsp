/**
 * Roland S-330 MIDI SysEx Client
 *
 * Universal client for communicating with Roland S-330 samplers via SysEx.
 * Works in both Node.js and browser environments via dependency injection.
 *
 * See /docs/s330_sysex.md for complete protocol documentation.
 *
 * ## Protocol Notes (from hardware testing, January 2026)
 *
 * The S-330 uses the RQD/WSD handshake protocol exclusively:
 *
 * - **RQD with address/size**: F0 41 dev 1E 41 [addr 4B] [size 4B] [cs] F7
 * - **WSD (Want to Send Data)**: Request permission to write parameters
 * - **DAT (Data Transfer)**: Bidirectional data packets with nibblized data
 * - **ACK (Acknowledge)**: Ready to receive next packet
 * - **EOD (End of Data)**: Transfer complete
 * - **ERR (Error)**: Communication error
 * - **RJC (Rejection)**: Request denied or no data available
 *
 * Important constraints:
 * - Both address LSB and size LSB must be EVEN (nibble alignment)
 * - Size represents nibble count, not byte count (size = bytes * 2)
 * - RQD handshake: RQD → DAT → ACK → ... → EOD → ACK (no initial ACK from device)
 * - Function parameter writes MUST use WSD/DAT/EOD (DT1 does not work)
 *
 * @packageDocumentation
 */

import type {
    S330MidiAdapter,
    S330SystemParams,
    S330Patch,
    S330Tone,
    S330ClientOptions,
    S330Response,
    S330Command,
    S330PatchCommon,
    S330AftertouchAssign,
    S330KeyAssign,
} from './s330-types.js';

import {
    ROLAND_ID,
    S330_MODEL_ID,
    DEFAULT_DEVICE_ID,
    S330_COMMANDS,
    TIMING,
    calculateChecksum,
    PATCH_TOTAL_SIZE,
    TONE_BLOCK_SIZE,
    TONE_STRIDE,
    MAX_PATCHES,
    MAX_TONES,
    PATCH_PARAMS,
    buildPatchParamAddress,
} from './s330-addresses.js';

import {
    parsePatchCommon,
    encodePatchCommon,
    parseTone,
    encodeTone,
    parseName,
    encodeAftertouchAssign,
    encodeKeyAssign,
    encodeSignedValue,
    createEmptyPatchCommon,
} from './s330-params.js';

import {
    parseDT1Message,
    type ParameterChangeEvent,
} from './s330-parameter-listener.js';

// =============================================================================
// Module-Level Cache
// =============================================================================

/**
 * Module-level cache for patch and tone data.
 * Sparse arrays - undefined slots indicate unloaded data.
 * Shared across all client instances - the cache is tied to the hardware state.
 */
let patchCache: (S330Patch | undefined)[] = new Array(MAX_PATCHES).fill(undefined);
let toneCache: (S330Tone | undefined)[] = new Array(MAX_TONES).fill(undefined);
let patchCacheInitialized = false;
let toneCacheInitialized = false;

// =============================================================================
// Data Types
// =============================================================================

/**
 * Data type codes for RQD/WSD commands
 */
export const S330_DATA_TYPES = {
    ALL_DATA: 0x00,
    PATCH_1_32: 0x01,
    PATCH_33_64: 0x02,
    TONE_1_32: 0x03,
    TONE_33_64: 0x04,
    FUNCTION: 0x05,
} as const;

export type S330DataType = (typeof S330_DATA_TYPES)[keyof typeof S330_DATA_TYPES];

/**
 * Function parameter addresses (00 01 00 xx)
 * Base address is 00 01 00 00 (function parameters bank)
 */
export const S330_FUNCTION_ADDRESSES = {
    // MULTI MIDI RX-CH (MIDI channel assignments 0-15 = Ch 1-16)
    MULTI_CHANNEL_A: 0x22,
    MULTI_CHANNEL_B: 0x23,
    MULTI_CHANNEL_C: 0x24,
    MULTI_CHANNEL_D: 0x25,
    MULTI_CHANNEL_E: 0x26,
    MULTI_CHANNEL_F: 0x27,
    MULTI_CHANNEL_G: 0x28,
    MULTI_CHANNEL_H: 0x29,

    // MULTI PATCH NUMBER (patch index 0-63)
    MULTI_PATCH_A: 0x32,
    MULTI_PATCH_B: 0x33,
    MULTI_PATCH_C: 0x34,
    MULTI_PATCH_D: 0x35,
    MULTI_PATCH_E: 0x36,
    MULTI_PATCH_F: 0x37,
    MULTI_PATCH_G: 0x38,
    MULTI_PATCH_H: 0x39,

    // MULTI OUTPUT ASSIGN (output 1-8, 0=mix)
    MULTI_OUTPUT_A: 0x42,
    MULTI_OUTPUT_B: 0x43,
    MULTI_OUTPUT_C: 0x44,
    MULTI_OUTPUT_D: 0x45,
    MULTI_OUTPUT_E: 0x46,
    MULTI_OUTPUT_F: 0x47,
    MULTI_OUTPUT_G: 0x48,
    MULTI_OUTPUT_H: 0x49,

    // MULTI LEVEL (level 0-127)
    MULTI_LEVEL_A: 0x56,
    MULTI_LEVEL_B: 0x57,
    MULTI_LEVEL_C: 0x58,
    MULTI_LEVEL_D: 0x59,
    MULTI_LEVEL_E: 0x5a,
    MULTI_LEVEL_F: 0x5b,
    MULTI_LEVEL_G: 0x5c,
    MULTI_LEVEL_H: 0x5d,
} as const;

/**
 * Patch name info returned from requestAllPatchNames
 */
export interface PatchNameInfo {
    index: number;
    name: string;
    isEmpty: boolean;
}

/**
 * Tone name info returned from requestAllToneNames
 */
export interface ToneNameInfo {
    index: number;
    name: string;
    isEmpty: boolean;
}

/**
 * Multi mode part configuration
 */
export interface MultiPartConfig {
    channel: number; // 0-15 (MIDI channel 1-16)
    patchIndex: number; // 0-63
    output: number; // 0-8 (0=mix, 1-8=individual outputs)
    level: number; // 0-127
}

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * De-nibblize data from DAT packets
 *
 * S-330 sends data with high nibble and low nibble separated:
 * [high0, low0, high1, low1, ...] → [byte0, byte1, ...]
 */
function deNibblize(nibbles: number[]): number[] {
    const bytes: number[] = [];
    for (let i = 0; i < nibbles.length - 1; i += 2) {
        const high = (nibbles[i] & 0x0f) << 4;
        const low = nibbles[i + 1] & 0x0f;
        bytes.push(high | low);
    }
    return bytes;
}

/**
 * Nibblize data for sending via DAT packets
 */
function nibblize(bytes: number[]): number[] {
    const nibbles: number[] = [];
    for (const byte of bytes) {
        nibbles.push((byte >> 4) & 0x0f);
        nibbles.push(byte & 0x0f);
    }
    return nibbles;
}


// =============================================================================
// S-330 Client Interface
// =============================================================================

/**
 * Progress callback for batch operations
 */
export type ProgressCallback = (current: number, total: number) => void;

/**
 * Callback for when an individual patch is loaded
 */
export type PatchLoadedCallback = (index: number, patch: S330Patch) => void;

/**
 * Callback for when an individual tone is loaded
 */
export type ToneLoadedCallback = (index: number, tone: S330Tone) => void;

/**
 * Callback for when a parameter changes on the hardware (front panel edits)
 */
export type ParameterChangeCallback = (event: ParameterChangeEvent) => void;

// Re-export the event type for consumers
export type { ParameterChangeEvent } from './s330-parameter-listener.js';

/**
 * S-330 client interface for MIDI communication
 *
 * This interface provides semantic methods for interacting with the S-330.
 * All address calculations and protocol details are handled internally.
 *
 * Data is cached at the module level - all reads go through the cache,
 * and writes update both cache and hardware.
 */
export interface S330ClientInterface {
    connect(): Promise<boolean>;
    disconnect(): void;
    isConnected(): boolean;
    getDeviceId(): number;

    // Patch operations (cached, single source of truth)
    getAllPatches(onProgress?: ProgressCallback, onPatchLoaded?: PatchLoadedCallback): Promise<S330Patch[]>;
    loadPatchRange(startIndex: number, count: number, onProgress?: ProgressCallback, onPatchLoaded?: PatchLoadedCallback, forceReload?: boolean): Promise<S330Patch[]>;
    getLoadedPatches(): (S330Patch | undefined)[];
    getPatch(patchIndex: number): Promise<S330Patch | null>;
    setPatch(patchIndex: number, patch: S330PatchCommon): Promise<void>;
    invalidatePatchCache(): void;

    // Tone operations (cached, single source of truth)
    getAllTones(onProgress?: ProgressCallback, onToneLoaded?: ToneLoadedCallback): Promise<S330Tone[]>;
    loadToneRange(startIndex: number, count: number, onProgress?: ProgressCallback, onToneLoaded?: ToneLoadedCallback, forceReload?: boolean): Promise<S330Tone[]>;
    getLoadedTones(): (S330Tone | undefined)[];
    getTone(toneIndex: number): Promise<S330Tone | null>;
    setTone(toneIndex: number, tone: S330Tone): Promise<void>;
    invalidateToneCache(): void;

    // Multi mode configuration (not cached)
    requestFunctionParameters(): Promise<MultiPartConfig[]>;
    setMultiChannel(part: number, channel: number): Promise<void>;
    setMultiPatch(part: number, patchIndex: number | null): Promise<void>;
    setMultiOutput(part: number, output: number): Promise<void>;
    setMultiLevel(part: number, level: number): Promise<void>;

    // MIDI panic - send All Notes Off / All Sound Off on all channels
    panic(): void;

    // Individual patch parameter setters (update cache + hardware)
    setPatchName(patchIndex: number, name: string): Promise<void>;
    setPatchKeyMode(
        patchIndex: number,
        keyMode: 'normal' | 'v-sw' | 'x-fade' | 'v-mix' | 'unison'
    ): Promise<void>;
    setPatchBenderRange(patchIndex: number, range: number): Promise<void>;
    setPatchAftertouchSens(patchIndex: number, sens: number): Promise<void>;
    setPatchAftertouchAssign(patchIndex: number, assign: S330AftertouchAssign): Promise<void>;
    setPatchKeyAssign(patchIndex: number, assign: S330KeyAssign): Promise<void>;
    setPatchOutput(patchIndex: number, output: number): Promise<void>;
    setPatchLevel(patchIndex: number, level: number): Promise<void>;
    setPatchDetune(patchIndex: number, detune: number): Promise<void>;
    setPatchVelocityThreshold(patchIndex: number, threshold: number): Promise<void>;
    setPatchVelocityMixRatio(patchIndex: number, ratio: number): Promise<void>;
    setPatchOctaveShift(patchIndex: number, shift: number): Promise<void>;

    // Hardware parameter change listening (for front panel sync)
    /**
     * Start listening for parameter changes from the hardware front panel.
     * When the user changes parameters on the S-330, DT1 messages are broadcast.
     * This method registers a listener to parse those messages and emit events.
     */
    startListening(): void;

    /**
     * Stop listening for parameter changes from the hardware.
     */
    stopListening(): void;

    /**
     * Check if currently listening for hardware parameter changes
     */
    isListening(): boolean;

    /**
     * Register a callback to be notified when parameters change on the hardware.
     * @param callback - Function to call with the parameter change event
     */
    onParameterChange(callback: ParameterChangeCallback): void;

    /**
     * Remove a previously registered parameter change callback.
     * @param callback - The callback function to remove
     */
    removeParameterChangeListener(callback: ParameterChangeCallback): void;
}

// =============================================================================
// Client Implementation
// =============================================================================

/**
 * Create S-330 MIDI client with dependency injection
 *
 * The client internally serializes all MIDI requests to prevent
 * concurrent requests from interfering with each other.
 *
 * @param midiAdapter - MIDI transport adapter (easymidi or Web MIDI)
 * @param options - Client configuration options
 * @returns S330ClientInterface instance
 *
 * @example Node.js with easymidi
 * ```typescript
 * import { createS330Client } from '@oletizi/sampler-devices/s330';
 * import { createEasymidiAdapter } from '@oletizi/sampler-midi';
 *
 * const adapter = createEasymidiAdapter('S-330');
 * const client = createS330Client(adapter, { deviceId: 0 });
 * await client.connect();
 * const patches = await client.requestAllPatchNames();
 * ```
 *
 * @example Browser with Web MIDI
 * ```typescript
 * import { createS330Client } from '@oletizi/sampler-devices/s330';
 * import { createWebMidiAdapter } from './midi/WebMidiAdapter';
 *
 * const adapter = await createWebMidiAdapter(inputPort, outputPort);
 * const client = createS330Client(adapter, { deviceId: 0 });
 * const patches = await client.requestAllPatchNames();
 * ```
 */
export function createS330Client(
    midiAdapter: S330MidiAdapter,
    options: S330ClientOptions = {}
): S330ClientInterface {
    const deviceId = options.deviceId ?? DEFAULT_DEVICE_ID;
    const timeoutMs = options.timeoutMs ?? TIMING.ACK_TIMEOUT_MS;
    const writeFlushDelayMs = options.writeFlushDelayMs ?? 150;

    let connected = false;

    // Request queue for serializing MIDI operations
    // The S-330 can only handle one request at a time
    let requestQueue: Promise<unknown> = Promise.resolve();

    /**
     * Serialize a request through the queue
     * Ensures only one MIDI request is in flight at a time
     */
    function serialize<T>(fn: () => Promise<T>): Promise<T> {
        const result = requestQueue.then(fn, fn); // Run even if previous failed
        requestQueue = result.catch(() => {}); // Swallow errors for queue chain
        return result;
    }

    // =========================================================================
    // Write Buffer - Rate limits device writes
    // =========================================================================

    /**
     * Buffer for pending writes.
     * Key is address as string (e.g., "0,2,4,0"), value is data to write.
     * Multiple writes to same address collapse to latest value.
     */
    const writeBuffer = new Map<string, { address: number[]; data: number[] }>();
    let flushTimer: ReturnType<typeof setTimeout> | null = null;
    let flushPromise: Promise<void> | null = null;

    /**
     * Convert address array to string key for buffer map
     */
    function addressKey(address: number[]): string {
        return address.join(',');
    }

    /**
     * Flush all buffered writes to the device.
     * Writes are sent sequentially through the serialize queue.
     */
    async function flushWriteBuffer(): Promise<void> {
        if (writeBuffer.size === 0) return;

        // Copy and clear buffer before async operations
        const pending = Array.from(writeBuffer.values());
        writeBuffer.clear();
        flushTimer = null;

        // Send all pending writes sequentially
        for (const { address, data } of pending) {
            await serialize(async () => {
                await sendData(address, data);
            });
        }
    }

    /**
     * Schedule a buffered write.
     * Multiple writes to the same address within writeFlushDelayMs are collapsed.
     *
     * @param address - 4-byte address
     * @param data - Data bytes to write
     * @returns Promise that resolves when the write is flushed
     */
    function bufferWrite(address: number[], data: number[]): Promise<void> {
        // If buffering is disabled, write immediately
        if (writeFlushDelayMs === 0) {
            return serialize(async () => {
                await sendData(address, data);
            });
        }

        // Add/update in buffer (collapses multiple writes to same address)
        const key = addressKey(address);
        writeBuffer.set(key, { address, data });

        // Schedule flush if not already scheduled
        if (!flushTimer) {
            flushPromise = new Promise((resolve, reject) => {
                flushTimer = setTimeout(() => {
                    flushWriteBuffer().then(resolve).catch(reject);
                }, writeFlushDelayMs);
            });
        }

        return flushPromise!;
    }

    /**
     * Send message and wait for response with timeout
     */
    function sendAndReceive(message: number[]): Promise<number[]> {
        return new Promise((resolve, reject) => {
            const timeout = setTimeout(() => {
                midiAdapter.removeSysExListener(listener);
                reject(new Error('S-330 response timeout'));
            }, timeoutMs);

            function listener(response: number[]) {
                if (
                    response.length >= 5 &&
                    response[1] === ROLAND_ID &&
                    response[2] === deviceId &&
                    response[3] === S330_MODEL_ID
                ) {
                    clearTimeout(timeout);
                    midiAdapter.removeSysExListener(listener);
                    resolve(response);
                }
            }

            midiAdapter.onSysEx(listener);
            midiAdapter.send(message);
        });
    }

    /**
     * Internal helper: Request data using RQD with address/size format
     *
     * This is the low-level method that handles the RQD handshake protocol.
     * Address and size constraints are enforced automatically:
     * - Address LSB must be even (aligned to nibble boundary)
     * - Size is converted to nibble count (bytes * 2)
     *
     * @param address - 4-byte address [aa, bb, cc, dd]
     * @param sizeInBytes - Number of logical bytes to request
     * @returns De-nibblized data bytes
     */
    async function requestDataWithAddress(
        address: number[],
        sizeInBytes: number
    ): Promise<number[]> {
        if (address.length !== 4) {
            throw new Error('Address must be 4 bytes');
        }

        // Ensure address LSB is even (nibble-aligned)
        if (address[3] & 0x01) {
            throw new Error(
                `Address LSB must be even (got 0x${address[3].toString(16)})`
            );
        }

        // Convert bytes to nibbles (size field represents nibble count)
        const sizeInNibbles = sizeInBytes * 2;

        // Build 4-byte size field (7-bit encoding per byte)
        const size = [
            (sizeInNibbles >> 21) & 0x7f,
            (sizeInNibbles >> 14) & 0x7f,
            (sizeInNibbles >> 7) & 0x7f,
            sizeInNibbles & 0x7f,
        ];

        // Calculate checksum over address + size
        const cs = calculateChecksum(address, size);

        const message = [
            0xf0,
            ROLAND_ID,
            deviceId,
            S330_MODEL_ID,
            S330_COMMANDS.RQD,
            ...address,
            ...size,
            cs,
            0xf7,
        ];

        return new Promise((resolve, reject) => {
            const allNibbles: number[] = [];
            let timeoutId: ReturnType<typeof setTimeout>;

            function resetTimeout() {
                if (timeoutId) clearTimeout(timeoutId);
                timeoutId = setTimeout(() => {
                    midiAdapter.removeSysExListener(listener);
                    if (allNibbles.length > 0) {
                        // Partial data received - return what we have
                        resolve(deNibblize(allNibbles));
                    } else {
                        reject(new Error('RQD response timeout - no data received'));
                    }
                }, timeoutMs * 2);
            }

            function sendAck() {
                const ackMsg = [
                    0xf0,
                    ROLAND_ID,
                    deviceId,
                    S330_MODEL_ID,
                    S330_COMMANDS.ACK,
                    0xf7,
                ];
                midiAdapter.send(ackMsg);
            }

            function listener(response: number[]) {
                console.log('[S330Client] Received:', response.slice(0, 10).map(b => b.toString(16).padStart(2, '0')).join(' '), response.length > 10 ? `... (${response.length} bytes)` : '');
                // Validate response header
                if (response.length < 5) return;
                if (response[1] !== ROLAND_ID) return;
                if (response[3] !== S330_MODEL_ID) return;

                const respDeviceId = response[2];
                const command = response[4];

                // Check device ID
                if (respDeviceId !== deviceId) {
                    if (command === S330_COMMANDS.RJC || command === S330_COMMANDS.ERR) {
                        clearTimeout(timeoutId);
                        midiAdapter.removeSysExListener(listener);
                        reject(
                            new Error(
                                `S-330 device ID mismatch (device: ${respDeviceId}, expected: ${deviceId})`
                            )
                        );
                    }
                    return;
                }

                resetTimeout();

                if (command === S330_COMMANDS.DAT) {
                    // DAT packet: F0 41 dev 1E 42 [addr 4B] [nibbles...] cs F7
                    // Data starts at byte 9 (after addr), ends 2 bytes before end (cs + F7)
                    const dataStart = 9;
                    const dataEnd = response.length - 2;
                    const nibbles = response.slice(dataStart, dataEnd);
                    allNibbles.push(...nibbles);
                    sendAck();
                } else if (command === S330_COMMANDS.EOD) {
                    clearTimeout(timeoutId);
                    midiAdapter.removeSysExListener(listener);
                    sendAck();
                    resolve(deNibblize(allNibbles));
                } else if (command === S330_COMMANDS.RJC) {
                    clearTimeout(timeoutId);
                    midiAdapter.removeSysExListener(listener);
                    reject(new Error('RQD request rejected by S-330'));
                } else if (command === S330_COMMANDS.ERR) {
                    clearTimeout(timeoutId);
                    midiAdapter.removeSysExListener(listener);
                    reject(new Error('S-330 communication error'));
                }
            }

            resetTimeout();
            midiAdapter.onSysEx(listener);
            console.log('[S330Client] Sending RQD:', message.map(b => b.toString(16).padStart(2, '0')).join(' '));
            midiAdapter.send(message);
        });
    }

    /**
     * Internal helper: Send data using WSD/DAT/EOD protocol
     *
     * This is the ONLY method that works for function parameter writes on S-330.
     * The protocol requires:
     * 1. Send WSD (Want to Send Data) with address and size
     * 2. Wait for ACK
     * 3. Send DAT with nibblized data
     * 4. Send EOD (End of Data)
     *
     * @param address - 4-byte address [aa, bb, cc, dd]
     * @param data - Parameter data bytes to write
     * @returns Promise that resolves when write is complete
     */
    async function sendData(address: number[], data: number[]): Promise<void> {
        if (address.length !== 4) {
            throw new Error('Address must be 4 bytes');
        }

        return new Promise((resolve, reject) => {
            // State machine: WSD → wait ACK → DAT → wait ACK → EOD → wait ACK → done
            let phase: 'WSD' | 'DAT' | 'EOD' | 'DONE' = 'WSD';
            const timeoutId = setTimeout(() => {
                midiAdapter.removeSysExListener(listener);
                reject(new Error(`WSD write timeout in phase ${phase}`));
            }, timeoutMs);

            function listener(response: number[]) {
                if (response.length < 5) return;
                if (response[1] !== ROLAND_ID) return;
                if (response[2] !== deviceId) return;
                if (response[3] !== S330_MODEL_ID) return;

                const command = response[4];

                if (command === S330_COMMANDS.RJC) {
                    clearTimeout(timeoutId);
                    midiAdapter.removeSysExListener(listener);
                    reject(new Error(`WSD rejected by S-330 in phase ${phase}`));
                    return;
                }
                if (command === S330_COMMANDS.ERR) {
                    clearTimeout(timeoutId);
                    midiAdapter.removeSysExListener(listener);
                    reject(new Error(`WSD communication error in phase ${phase}`));
                    return;
                }

                if (command === S330_COMMANDS.ACK) {
                    if (phase === 'WSD') {
                        // Received ACK for WSD - send DAT
                        phase = 'DAT';

                        const nibblized = nibblize(data);
                        const datChecksum = calculateChecksum(address, nibblized);

                        const datMessage = [
                            0xf0,
                            ROLAND_ID,
                            deviceId,
                            S330_MODEL_ID,
                            S330_COMMANDS.DAT,
                            ...address,
                            ...nibblized,
                            datChecksum,
                            0xf7,
                        ];

                        midiAdapter.send(datMessage);
                    } else if (phase === 'DAT') {
                        // Received ACK for DAT - send EOD
                        phase = 'EOD';

                        const eodMessage = [
                            0xf0,
                            ROLAND_ID,
                            deviceId,
                            S330_MODEL_ID,
                            S330_COMMANDS.EOD,
                            0xf7,
                        ];

                        midiAdapter.send(eodMessage);
                    } else if (phase === 'EOD') {
                        // Received ACK for EOD - write complete
                        phase = 'DONE';
                        clearTimeout(timeoutId);
                        midiAdapter.removeSysExListener(listener);
                        resolve();
                    }
                }
            }

            midiAdapter.onSysEx(listener);

            // Build and send WSD message
            // Size is in nibbles (same as RQD) - each byte becomes 2 nibbles
            const sizeInNibbles = data.length * 2;
            const size = [
                (sizeInNibbles >> 21) & 0x7f,
                (sizeInNibbles >> 14) & 0x7f,
                (sizeInNibbles >> 7) & 0x7f,
                sizeInNibbles & 0x7f,
            ];

            const wsdChecksum = calculateChecksum(address, size);

            const wsdMessage = [
                0xf0,
                ROLAND_ID,
                deviceId,
                S330_MODEL_ID,
                S330_COMMANDS.WSD,
                ...address,
                ...size,
                wsdChecksum,
                0xf7,
            ];

            midiAdapter.send(wsdMessage);
        });
    }

    // =========================================================================
    // Parameter Change Listening
    // =========================================================================

    /**
     * Registered callbacks for parameter change events
     */
    const parameterListeners = new Set<ParameterChangeCallback>();

    /**
     * Whether we're currently listening for hardware parameter changes
     */
    let listening = false;

    /**
     * The SysEx listener function for parameter changes
     * Stored so we can remove it when stopListening() is called
     */
    function parameterSysExListener(message: number[]) {
        // Log incoming message for debugging
        console.log('[S330Client] parameterSysExListener received:', message.length, 'bytes',
            message.slice(0, 10).map(b => b.toString(16).padStart(2, '0')).join(' '));

        // Parse the message to see if it's a DT1 from the S-330
        const result = parseDT1Message(message, deviceId);

        console.log('[S330Client] parseDT1Message result:', result.valid, result.reason || '');

        if (result.valid && result.event) {
            console.log('[S330Client] Valid DT1 event:', result.event.type, 'index:', result.event.index);
            // Notify all registered listeners
            for (const callback of parameterListeners) {
                try {
                    callback(result.event);
                } catch (err) {
                    console.error('[S330Client] Parameter change callback error:', err);
                }
            }
        }
    }

    return {
        async connect(): Promise<boolean> {
            connected = true;
            return true;
        },

        disconnect(): void {
            connected = false;
        },

        isConnected(): boolean {
            return connected;
        },

        getDeviceId(): number {
            return deviceId;
        },

        /**
         * Send MIDI panic: All Sound Off (CC#120) and All Notes Off (CC#123) on all channels.
         * This immediately silences all notes without waiting for the message queue.
         * Use when notes get stuck during heavy SysEx traffic.
         */
        panic(): void {
            // Send All Sound Off (CC#120) and All Notes Off (CC#123) on all 16 channels
            for (let channel = 0; channel < 16; channel++) {
                const status = 0xb0 + channel; // Control Change status byte
                // CC#120 - All Sound Off (immediate silence)
                midiAdapter.send([status, 120, 0]);
                // CC#123 - All Notes Off (release all held notes)
                midiAdapter.send([status, 123, 0]);
            }
            console.log('[S330Client] Panic: sent All Sound Off and All Notes Off on all channels');
        },

        // =====================================================================
        // Patch Operations (Cached)
        // =====================================================================

        /**
         * Load a range of patches from the S-330.
         * Only fetches patches that aren't already cached.
         * @param startIndex - Starting patch index (0-63)
         * @param count - Number of patches to load
         * @param onProgress - Optional callback for progress updates (current, total)
         */
        async loadPatchRange(startIndex: number, count: number, onProgress?: ProgressCallback, onPatchLoaded?: PatchLoadedCallback, forceReload?: boolean): Promise<S330Patch[]> {
            const endIndex = Math.min(startIndex + count, MAX_PATCHES);
            const actualCount = endIndex - startIndex;

            // If forcing reload, clear cache for this range
            if (forceReload) {
                for (let i = startIndex; i < endIndex; i++) {
                    patchCache[i] = undefined;
                }
            }

            return serialize(async () => {
                console.log(`[S330Client] Loading patches ${startIndex}-${endIndex - 1}${forceReload ? ' (forced)' : ''}...`);
                const result: S330Patch[] = [];
                let loaded = 0;

                for (let i = startIndex; i < endIndex; i++) {
                    // Skip if already cached
                    if (patchCache[i] !== undefined) {
                        const patch = patchCache[i]!;
                        result.push(patch);
                        loaded++;
                        onProgress?.(loaded, actualCount);
                        onPatchLoaded?.(i, patch);
                        continue;
                    }

                    let patch: S330Patch;
                    try {
                        const address = [0x00, 0x00, i * 4, 0x00];
                        const data = await requestDataWithAddress(address, PATCH_TOTAL_SIZE);
                        const common = parsePatchCommon(data);
                        patch = { common };
                    } catch {
                        // Create placeholder for failed reads
                        const placeholder = createEmptyPatchCommon(i);
                        placeholder.name = '!ERROR!';
                        patch = { common: placeholder };
                    }
                    patchCache[i] = patch;
                    result.push(patch);
                    loaded++;
                    onProgress?.(loaded, actualCount);
                    onPatchLoaded?.(i, patch);
                }

                patchCacheInitialized = true;
                console.log(`[S330Client] Loaded ${result.length} patches (${startIndex}-${endIndex - 1})`);
                return result;
            });
        },

        /**
         * Get all patches from the S-330, using cache if available.
         * Fetches full patch data for all patches.
         * @param onProgress - Optional callback for progress updates (current, total)
         * @param onPatchLoaded - Optional callback called after each patch is loaded
         */
        async getAllPatches(onProgress?: ProgressCallback, onPatchLoaded?: PatchLoadedCallback): Promise<S330Patch[]> {
            // Check if all patches are cached
            const allCached = patchCacheInitialized && patchCache.every(p => p !== undefined);
            if (allCached) {
                return patchCache as S330Patch[];
            }

            // Load all patches
            await this.loadPatchRange(0, MAX_PATCHES, onProgress, onPatchLoaded);
            return patchCache as S330Patch[];
        },

        /**
         * Get currently loaded patches (sparse array, undefined = not loaded)
         */
        getLoadedPatches(): (S330Patch | undefined)[] {
            return [...patchCache];
        },

        /**
         * Get a single patch by index, using cache if available.
         * Loads from hardware if not cached.
         */
        async getPatch(patchIndex: number): Promise<S330Patch | null> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            // Return cached if available
            if (patchCache[patchIndex] !== undefined) {
                return patchCache[patchIndex]!;
            }

            // Load just this one patch
            const result = await this.loadPatchRange(patchIndex, 1);
            return result[0] ?? null;
        },

        /**
         * Set a patch - updates cache AND writes to hardware.
         */
        async setPatch(patchIndex: number, patch: S330PatchCommon): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            // Update cache
            patchCache[patchIndex] = { common: patch };

            // Then write to hardware
            const address = [0x00, 0x00, patchIndex * 4, 0x00];
            const patchBytes = encodePatchCommon(patch);
            return bufferWrite(address, patchBytes);
        },

        /**
         * Invalidate the patch cache, forcing a reload on next access.
         */
        invalidatePatchCache(): void {
            console.log('[S330Client] Invalidating patch cache');
            patchCache = new Array(MAX_PATCHES).fill(undefined);
            patchCacheInitialized = false;
        },

        // =====================================================================
        // Tone Operations (Cached)
        // =====================================================================

        /**
         * Load a range of tones from the S-330.
         * Only fetches tones that aren't already cached.
         * @param startIndex - Starting tone index (0-31)
         * @param count - Number of tones to load
         * @param onProgress - Optional callback for progress updates (current, total)
         * @param onToneLoaded - Optional callback called after each tone is loaded
         */
        async loadToneRange(startIndex: number, count: number, onProgress?: ProgressCallback, onToneLoaded?: ToneLoadedCallback, forceReload?: boolean): Promise<S330Tone[]> {
            const endIndex = Math.min(startIndex + count, MAX_TONES);
            const actualCount = endIndex - startIndex;

            // If forcing reload, clear cache for this range
            if (forceReload) {
                for (let i = startIndex; i < endIndex; i++) {
                    toneCache[i] = undefined;
                }
            }

            return serialize(async () => {
                console.log(`[S330Client] Loading tones ${startIndex}-${endIndex - 1}${forceReload ? ' (forced)' : ''}...`);
                const result: S330Tone[] = [];
                let loaded = 0;

                for (let i = startIndex; i < endIndex; i++) {
                    // Skip if already cached
                    if (toneCache[i] !== undefined) {
                        const tone = toneCache[i]!;
                        result.push(tone);
                        loaded++;
                        onProgress?.(loaded, actualCount);
                        onToneLoaded?.(i, tone);
                        continue;
                    }

                    let tone: S330Tone;
                    try {
                        const byte2 = i * 2;
                        const address = [0x00, 0x03, byte2, 0x00];
                        const data = await requestDataWithAddress(address, TONE_BLOCK_SIZE);
                        tone = parseTone(data);
                    } catch {
                        // Create placeholder for failed reads
                        tone = parseTone(new Array(TONE_BLOCK_SIZE).fill(0));
                        tone.name = '!ERROR!';
                    }
                    toneCache[i] = tone;
                    result.push(tone);
                    loaded++;
                    onProgress?.(loaded, actualCount);
                    onToneLoaded?.(i, tone);
                }

                toneCacheInitialized = true;
                console.log(`[S330Client] Loaded ${result.length} tones (${startIndex}-${endIndex - 1})`);
                return result;
            });
        },

        /**
         * Get all tones from the S-330, using cache if available.
         * Fetches full tone data for all 32 tones.
         * @param onProgress - Optional callback for progress updates (current, total)
         * @param onToneLoaded - Optional callback called after each tone is loaded
         */
        async getAllTones(onProgress?: ProgressCallback, onToneLoaded?: ToneLoadedCallback): Promise<S330Tone[]> {
            // Check if all tones are cached
            const allCached = toneCacheInitialized && toneCache.every(t => t !== undefined);
            if (allCached) {
                return toneCache as S330Tone[];
            }

            // Load all tones
            await this.loadToneRange(0, MAX_TONES, onProgress, onToneLoaded);
            return toneCache as S330Tone[];
        },

        /**
         * Get currently loaded tones (sparse array, undefined = not loaded)
         */
        getLoadedTones(): (S330Tone | undefined)[] {
            return [...toneCache];
        },

        /**
         * Get a single tone by index, using cache if available.
         * Loads from hardware if not cached.
         */
        async getTone(toneIndex: number): Promise<S330Tone | null> {
            if (toneIndex < 0 || toneIndex >= MAX_TONES) {
                throw new Error(`Invalid tone index: ${toneIndex}`);
            }

            // Return cached if available
            if (toneCache[toneIndex] !== undefined) {
                return toneCache[toneIndex]!;
            }

            // Load just this one tone
            const result = await this.loadToneRange(toneIndex, 1);
            return result[0] ?? null;
        },

        /**
         * Set a tone - updates cache AND writes to hardware.
         */
        async setTone(toneIndex: number, tone: S330Tone): Promise<void> {
            if (toneIndex < 0 || toneIndex >= MAX_TONES) {
                throw new Error(`Invalid tone index: ${toneIndex}`);
            }

            // Update cache
            toneCache[toneIndex] = tone;

            // Then write to hardware
            const byte2 = toneIndex * 2;
            const address = [0x00, 0x03, byte2, 0x00];
            const toneBytes = encodeTone(tone);
            return bufferWrite(address, toneBytes);
        },

        /**
         * Invalidate the tone cache, forcing a reload on next access.
         */
        invalidateToneCache(): void {
            console.log('[S330Client] Invalidating tone cache');
            toneCache = new Array(MAX_TONES).fill(undefined);
            toneCacheInitialized = false;
        },

        /**
         * Request all function parameters (Multi mode configuration)
         * Serialized to prevent concurrent request interference
         *
         * Address base: 00 01 00 00 (function parameters bank per MIDI Implementation)
         * Returns configuration for all 8 parts (A-H)
         */
        async requestFunctionParameters(): Promise<MultiPartConfig[]> {
            return serialize(async () => {
                try {
                    // Read each parameter group separately
                    const channelData = await requestDataWithAddress(
                        [0x00, 0x01, 0x00, 0x22],
                        8
                    );
                    const patchData = await requestDataWithAddress(
                        [0x00, 0x01, 0x00, 0x32],
                        8
                    );

                    let outputData: number[];
                    try {
                        outputData = await requestDataWithAddress(
                            [0x00, 0x01, 0x00, 0x42],
                            8
                        );
                    } catch (err) {
                        // Output parameters may not exist on all firmware versions
                        outputData = [1, 1, 1, 1, 1, 1, 1, 1];
                    }

                    const levelData = await requestDataWithAddress(
                        [0x00, 0x01, 0x00, 0x56],
                        8
                    );

                    const parts: MultiPartConfig[] = [];
                    for (let i = 0; i < 8; i++) {
                        parts.push({
                            channel: channelData[i] ?? 0,
                            patchIndex: patchData[i] ?? 0,
                            output: outputData[i] ?? 1,
                            level: levelData[i] ?? 127,
                        });
                    }

                    return parts;
                } catch (err) {
                    // Return default configuration
                    return Array.from({ length: 8 }, (_, i) => ({
                        channel: i,
                        patchIndex: 0,
                        output: 1,
                        level: 127,
                    }));
                }
            });
        },

        /**
         * Set MIDI receive channel for a multi mode part
         * Uses WSD/DAT/EOD protocol (DT1 does not work for function parameters)
         */
        async setMultiChannel(part: number, channel: number): Promise<void> {
            if (part < 0 || part > 7) {
                throw new Error(`Invalid part number: ${part} (must be 0-7)`);
            }
            if (channel < 0 || channel > 15) {
                throw new Error(`Invalid channel: ${channel} (must be 0-15)`);
            }

            return serialize(async () => {
                const allChannelData = await requestDataWithAddress(
                    [0x00, 0x01, 0x00, 0x22],
                    8
                );
                const newChannelData = [...allChannelData];
                newChannelData[part] = channel;
                await sendData([0x00, 0x01, 0x00, 0x22], newChannelData);
            });
        },

        /**
         * Set patch assignment for a multi mode part
         * Uses WSD/DAT/EOD protocol (DT1 does not work for function parameters)
         */
        async setMultiPatch(part: number, patchIndex: number | null): Promise<void> {
            if (part < 0 || part > 7) {
                throw new Error(`Invalid part number: ${part} (must be 0-7)`);
            }
            const value = patchIndex === null ? 0x7f : patchIndex;
            if (patchIndex !== null && (patchIndex < 0 || patchIndex > 63)) {
                throw new Error(`Invalid patch index: ${patchIndex} (must be 0-63 or null)`);
            }

            return serialize(async () => {
                const allPatchData = await requestDataWithAddress(
                    [0x00, 0x01, 0x00, 0x32],
                    8
                );
                const newPatchData = [...allPatchData];
                newPatchData[part] = value;
                await sendData([0x00, 0x01, 0x00, 0x32], newPatchData);
            });
        },

        /**
         * Set output assignment for a multi mode part
         * Uses WSD/DAT/EOD protocol (DT1 does not work for function parameters)
         */
        async setMultiOutput(part: number, output: number): Promise<void> {
            if (part < 0 || part > 7) {
                throw new Error(`Invalid part number: ${part} (must be 0-7)`);
            }
            if (output < 0 || output > 8) {
                throw new Error(`Invalid output: ${output} (must be 0-8)`);
            }

            return serialize(async () => {
                let allOutputData: number[];
                try {
                    allOutputData = await requestDataWithAddress(
                        [0x00, 0x01, 0x00, 0x42],
                        8
                    );
                } catch (err) {
                    allOutputData = [1, 1, 1, 1, 1, 1, 1, 1];
                }
                const newOutputData = [...allOutputData];
                newOutputData[part] = output;
                await sendData([0x00, 0x01, 0x00, 0x42], newOutputData);
            });
        },

        /**
         * Set level for a multi mode part
         * Uses WSD/DAT/EOD protocol (DT1 does not work for function parameters)
         */
        async setMultiLevel(part: number, level: number): Promise<void> {
            if (part < 0 || part > 7) {
                throw new Error(`Invalid part number: ${part} (must be 0-7)`);
            }
            if (level < 0 || level > 127) {
                throw new Error(`Invalid level: ${level} (must be 0-127)`);
            }

            return serialize(async () => {
                const allLevelData = await requestDataWithAddress(
                    [0x00, 0x01, 0x00, 0x56],
                    8
                );
                const newLevelData = [...allLevelData];
                newLevelData[part] = level;
                await sendData([0x00, 0x01, 0x00, 0x56], newLevelData);
            });
        },

        /**
         * Set patch name
         * Uses 2-byte writes in 8-byte chunks at the exact name address
         * @param patchIndex - Patch number (0-63)
         * @param name - Patch name (max 12 characters, padded with spaces)
         */
        async setPatchName(patchIndex: number, name: string): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            return serialize(async () => {
                // Get the exact address for the name parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.name);

                // Pad name with spaces to 12 characters
                const nameBytes = Array(PATCH_PARAMS.name.size).fill(0x20); // ASCII space
                for (let i = 0; i < Math.min(name.length, PATCH_PARAMS.name.size); i++) {
                    nameBytes[i] = name.charCodeAt(i);
                }

                // Write name in 8-byte chunk (first 8 chars) then 4-byte chunk (last 4 chars)
                // S-330 accepts 8-byte writes for patch bank
                await sendData(address, nameBytes.slice(0, 8));

                // Calculate address for remaining 4 bytes (offset by 8 nibble pairs = 16 nibbles = 0x10)
                const secondAddress = [...address];
                secondAddress[3] = (secondAddress[3] + 0x10) & 0x7f; // Advance by 8 bytes (16 nibbles)

                // Write remaining 4 bytes (but we need at least 2 bytes for WSD)
                await sendData(secondAddress, nameBytes.slice(8, 12));
            });
        },

        /**
         * Set key mode for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchKeyMode(
            patchIndex: number,
            keyMode: 'normal' | 'v-sw' | 'x-fade' | 'v-mix' | 'unison'
        ): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            const keyModeValue =
                keyMode === 'normal'
                    ? 0
                    : keyMode === 'v-sw'
                    ? 1
                    : keyMode === 'x-fade'
                    ? 2
                    : keyMode === 'v-mix'
                    ? 3
                    : 4;

            return serialize(async () => {
                // Get the exact address for the key mode parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.keyMode);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the key mode (first byte)
                newData[0] = keyModeValue;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set bender range for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchBenderRange(patchIndex: number, range: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (range < 0 || range > 12) {
                throw new Error(`Invalid bender range: ${range} (must be 0-12)`);
            }

            return serialize(async () => {
                // Get the exact address for the bender range parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.benderRange);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the bender range (first byte)
                newData[0] = range;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set aftertouch sensitivity for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchAftertouchSens(patchIndex: number, sens: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (sens < 0 || sens > 127) {
                throw new Error(`Invalid aftertouch sensitivity: ${sens}`);
            }

            return serialize(async () => {
                // Get the exact address for the aftertouch sensitivity parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.aftertouchSens);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the aftertouch sensitivity (first byte)
                newData[0] = sens;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set output assignment for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchOutput(patchIndex: number, output: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (output < 0 || output > 8) {
                throw new Error(`Invalid output: ${output} (must be 0-8)`);
            }

            return serialize(async () => {
                // Get the exact address for the output assignment parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.outputAssign);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the output assignment (first byte)
                newData[0] = output;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set level for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchLevel(patchIndex: number, level: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (level < 0 || level > 127) {
                throw new Error(`Invalid level: ${level}`);
            }

            return serialize(async () => {
                // Get the exact address for the level parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.level);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the level (first byte)
                newData[0] = level;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set detune for a patch (unison mode)
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         * @param detune - Detune amount (-64 to +63, displayed as -64 to +63)
         */
        async setPatchDetune(patchIndex: number, detune: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (detune < -64 || detune > 63) {
                throw new Error(`Invalid detune: ${detune} (must be -64 to +63)`);
            }

            return serialize(async () => {
                // Get the exact address for the detune parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.detune);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the detune (first byte) - uses signed encoding
                newData[0] = encodeSignedValue(detune);

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set velocity threshold for a patch (V-Sw mode)
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         * @param threshold - Velocity threshold (0-127)
         */
        async setPatchVelocityThreshold(patchIndex: number, threshold: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (threshold < 0 || threshold > 127) {
                throw new Error(`Invalid velocity threshold: ${threshold}`);
            }

            return serialize(async () => {
                // Get the exact address for the velocity threshold parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.velocityThreshold);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the velocity threshold (first byte)
                newData[0] = threshold;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set velocity mix ratio for a patch (V-Mix mode)
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         * @param ratio - Velocity mix ratio (0-127)
         */
        async setPatchVelocityMixRatio(patchIndex: number, ratio: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (ratio < 0 || ratio > 127) {
                throw new Error(`Invalid velocity mix ratio: ${ratio}`);
            }

            return serialize(async () => {
                // Get the exact address for the velocity mix ratio parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.velocityMixRatio);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the velocity mix ratio (first byte)
                newData[0] = ratio;

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set octave shift for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         * @param shift - Octave shift (-2 to +2)
         */
        async setPatchOctaveShift(patchIndex: number, shift: number): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }
            if (shift < -2 || shift > 2) {
                throw new Error(`Invalid octave shift: ${shift} (must be -2 to +2)`);
            }

            return serialize(async () => {
                // Get the exact address for the octave shift parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.octaveShift);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the octave shift (first byte) - uses signed encoding
                newData[0] = encodeSignedValue(shift, 2);

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set aftertouch assignment for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchAftertouchAssign(patchIndex: number, assign: S330AftertouchAssign): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            return serialize(async () => {
                // Get the exact address for the aftertouch assign parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.aftertouchAssign);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the aftertouch assign (first byte)
                newData[0] = encodeAftertouchAssign(assign);

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        /**
         * Set key assignment mode for a patch
         * Uses 8-byte read-modify-write (S-330 requires 8-byte minimum for WSD)
         */
        async setPatchKeyAssign(patchIndex: number, assign: S330KeyAssign): Promise<void> {
            if (patchIndex < 0 || patchIndex >= MAX_PATCHES) {
                throw new Error(`Invalid patch index: ${patchIndex}`);
            }

            return serialize(async () => {
                // Get the exact address for the key assign parameter
                const address = buildPatchParamAddress(patchIndex, PATCH_PARAMS.keyAssign);

                // Read 8 bytes at this address (S-330 requires 8-byte writes)
                const data = await requestDataWithAddress(address, 8);
                const newData = [...data];

                // Modify the key assign (first byte)
                newData[0] = encodeKeyAssign(assign);

                // Write 8 bytes back
                await sendData(address, newData);
            });
        },

        // =====================================================================
        // Hardware Parameter Change Listening
        // =====================================================================

        /**
         * Start listening for parameter changes from the hardware front panel.
         * When the user changes parameters on the S-330, DT1 messages are broadcast.
         */
        startListening(): void {
            if (listening) return;

            midiAdapter.onSysEx(parameterSysExListener);
            listening = true;
            console.log('[S330Client] Started listening for hardware parameter changes');
        },

        /**
         * Stop listening for parameter changes from the hardware.
         */
        stopListening(): void {
            if (!listening) return;

            midiAdapter.removeSysExListener(parameterSysExListener);
            listening = false;
            console.log('[S330Client] Stopped listening for hardware parameter changes');
        },

        /**
         * Check if currently listening for hardware parameter changes
         */
        isListening(): boolean {
            return listening;
        },

        /**
         * Register a callback to be notified when parameters change on the hardware.
         * @param callback - Function to call with the parameter change event
         */
        onParameterChange(callback: ParameterChangeCallback): void {
            parameterListeners.add(callback);
        },

        /**
         * Remove a previously registered parameter change callback.
         * @param callback - The callback function to remove
         */
        removeParameterChangeListener(callback: ParameterChangeCallback): void {
            parameterListeners.delete(callback);
        },

    };
}
