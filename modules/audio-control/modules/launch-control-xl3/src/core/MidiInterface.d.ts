/**
 * Platform-agnostic MIDI interface
 *
 * Provides abstraction layer for different MIDI backends (node-midi, Web MIDI API, etc.)
 */
import { EventEmitter } from 'eventemitter3';
export interface MidiMessage {
    readonly timestamp: number;
    readonly data: readonly number[];
    type?: string;
    channel?: number;
    controller?: number;
    value?: number;
    note?: number;
    velocity?: number;
}
export interface MidiPortInfo {
    readonly id: string;
    readonly name: string;
    readonly manufacturer?: string;
    readonly version?: string;
}
export interface MidiPort {
    readonly id: string;
    readonly name: string;
    readonly type: 'input' | 'output';
    close(): Promise<void>;
}
export interface MidiInputPort extends MidiPort {
    readonly type: 'input';
    onMessage?: ((message: MidiMessage) => void) | undefined;
}
export interface MidiOutputPort extends MidiPort {
    readonly type: 'output';
}
export interface MidiBackendInterface {
    initialize(): Promise<void>;
    getInputPorts(): Promise<MidiPortInfo[]>;
    getOutputPorts(): Promise<MidiPortInfo[]>;
    openInput(portId: string): Promise<MidiInputPort>;
    openOutput(portId: string): Promise<MidiOutputPort>;
    sendMessage(port: MidiOutputPort, message: MidiMessage): Promise<void>;
    closePort(port: MidiPort): Promise<void>;
    cleanup(): Promise<void>;
}
export interface MidiInterfaceEvents {
    message: (message: MidiMessage) => void;
    connected: (port: MidiPort) => void;
    disconnected: (port: MidiPort) => void;
    error: (error: Error) => void;
}
/**
 * Main MIDI interface class
 * Manages MIDI connections and message routing
 */
export declare class MidiInterface extends EventEmitter {
    private backend?;
    private inputPort?;
    private outputPort?;
    private isInitialized;
    private messageBuffer;
    private readonly maxBufferSize;
    constructor(backend?: MidiBackendInterface);
    /**
     * Initialize the MIDI interface
     */
    initialize(): Promise<void>;
    /**
     * Get available input ports
     */
    getInputPorts(): Promise<MidiPortInfo[]>;
    /**
     * Get available output ports
     */
    getOutputPorts(): Promise<MidiPortInfo[]>;
    /**
     * Open input port by ID or name
     */
    openInput(portIdOrName: string): Promise<void>;
    /**
     * Open output port by ID or name
     */
    openOutput(portIdOrName: string): Promise<void>;
    /**
     * Send MIDI message
     */
    sendMessage(data: number[]): Promise<void>;
    /**
     * Close input port
     */
    closeInput(): Promise<void>;
    /**
     * Close output port
     */
    closeOutput(): Promise<void>;
    /**
     * Close all ports and cleanup
     */
    cleanup(): Promise<void>;
    /**
     * Get current connection status
     */
    isConnected(): boolean;
    /**
     * Get buffered messages
     */
    getBufferedMessages(): MidiMessage[];
    /**
     * Clear message buffer
     */
    clearBuffer(): void;
    /**
     * Handle incoming MIDI message
     */
    private handleIncomingMessage;
}
//# sourceMappingURL=MidiInterface.d.ts.map