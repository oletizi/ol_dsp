/**
 * JUCE MIDI Backend - Proxies MIDI through external JUCE server
 *
 * This backend communicates with a JUCE-based MIDI server to handle
 * all MIDI operations, avoiding the limitations of Node.js MIDI libraries.
 */
import { MidiBackendInterface, MidiPortInfo, MidiInputPort, MidiOutputPort, MidiMessage, MidiPort } from '../core/MidiInterface.js';
import { EventEmitter } from 'events';
interface JuceServerConfig {
    host: string;
    port: number;
}
export declare class JuceMidiBackend extends EventEmitter implements MidiBackendInterface {
    private config;
    private pollInterval;
    private openPorts;
    constructor(config?: JuceServerConfig);
    private get baseUrl();
    initialize(): Promise<void>;
    getInputPorts(): Promise<MidiPortInfo[]>;
    getOutputPorts(): Promise<MidiPortInfo[]>;
    openInput(portId: string): Promise<MidiInputPort>;
    openOutput(portId: string): Promise<MidiOutputPort>;
    sendMessage(port: MidiOutputPort, message: MidiMessage): Promise<void>;
    closePort(port: MidiPort): Promise<void>;
    cleanup(): Promise<void>;
    private startPolling;
    private stopPolling;
    close(): Promise<void>;
    dispose(): void;
}
export {};
//# sourceMappingURL=JuceMidiBackend.d.ts.map