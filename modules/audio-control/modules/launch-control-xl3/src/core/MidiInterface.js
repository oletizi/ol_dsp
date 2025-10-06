/**
 * Platform-agnostic MIDI interface
 *
 * Provides abstraction layer for different MIDI backends (node-midi, Web MIDI API, etc.)
 */
import { EventEmitter } from 'eventemitter3';
/**
 * Main MIDI interface class
 * Manages MIDI connections and message routing
 */
export class MidiInterface extends EventEmitter {
    backend;
    inputPort;
    outputPort;
    isInitialized = false;
    messageBuffer = [];
    maxBufferSize = 1000;
    constructor(backend) {
        super();
        this.backend = backend;
    }
    /**
     * Initialize the MIDI interface
     */
    async initialize() {
        if (this.isInitialized) {
            return;
        }
        // If no backend provided, try to auto-detect
        if (!this.backend) {
            throw new Error('MIDI backend is not initialized');
            //this.backend = await this.autoDetectBackend();
        }
        await this.backend.initialize();
        this.isInitialized = true;
    }
    /**
     * Get available input ports
     */
    async getInputPorts() {
        if (!this.isInitialized) {
            throw new Error('MidiInterface not initialized');
        }
        if (!this.backend) {
            throw new Error('No MIDI backend available');
        }
        return this.backend.getInputPorts();
    }
    /**
     * Get available output ports
     */
    async getOutputPorts() {
        if (!this.isInitialized) {
            throw new Error('MidiInterface not initialized');
        }
        if (!this.backend) {
            throw new Error('No MIDI backend available');
        }
        return this.backend.getOutputPorts();
    }
    /**
     * Open input port by ID or name
     */
    async openInput(portIdOrName) {
        if (!this.isInitialized) {
            throw new Error('MidiInterface not initialized');
        }
        if (!this.backend) {
            throw new Error('No MIDI backend available');
        }
        // Close existing input if open
        if (this.inputPort) {
            await this.closeInput();
        }
        // Find port
        const ports = await this.backend.getInputPorts();
        const port = ports.find((p) => p.id === portIdOrName || p.name === portIdOrName);
        if (!port) {
            throw new Error(`Input port not found: ${portIdOrName}`);
        }
        // Open port
        this.inputPort = await this.backend.openInput(port.id);
        // Set up message handler
        this.inputPort.onMessage = (message) => {
            this.handleIncomingMessage(message);
        };
        this.emit('connected', this.inputPort);
    }
    /**
     * Open output port by ID or name
     */
    async openOutput(portIdOrName) {
        if (!this.isInitialized) {
            throw new Error('MidiInterface not initialized');
        }
        if (!this.backend) {
            throw new Error('No MIDI backend available');
        }
        // Close existing output if open
        if (this.outputPort) {
            await this.closeOutput();
        }
        // Find port
        const ports = await this.backend.getOutputPorts();
        const port = ports.find((p) => p.id === portIdOrName || p.name === portIdOrName);
        if (!port) {
            throw new Error(`Output port not found: ${portIdOrName}`);
        }
        // Open port
        this.outputPort = await this.backend.openOutput(port.id);
        this.emit('connected', this.outputPort);
    }
    /**
     * Send MIDI message
     */
    async sendMessage(data) {
        if (!this.outputPort) {
            throw new Error('No output port open');
        }
        if (!this.backend) {
            throw new Error('No MIDI backend available');
        }
        const message = {
            timestamp: Date.now(),
            data,
        };
        await this.backend.sendMessage(this.outputPort, message);
    }
    /**
     * Close input port
     */
    async closeInput() {
        if (this.inputPort && this.backend) {
            await this.backend.closePort(this.inputPort);
            this.emit('disconnected', this.inputPort);
            this.inputPort = undefined;
        }
    }
    /**
     * Close output port
     */
    async closeOutput() {
        if (this.outputPort && this.backend) {
            await this.backend.closePort(this.outputPort);
            this.emit('disconnected', this.outputPort);
            this.outputPort = undefined;
        }
    }
    /**
     * Close all ports and cleanup
     */
    async cleanup() {
        await this.closeInput();
        await this.closeOutput();
        if (this.backend) {
            await this.backend.cleanup();
        }
        this.isInitialized = false;
        this.messageBuffer = [];
    }
    /**
     * Get current connection status
     */
    isConnected() {
        return !!(this.inputPort || this.outputPort);
    }
    /**
     * Get buffered messages
     */
    getBufferedMessages() {
        return [...this.messageBuffer];
    }
    /**
     * Clear message buffer
     */
    clearBuffer() {
        this.messageBuffer = [];
    }
    /**
     * Handle incoming MIDI message
     */
    handleIncomingMessage(message) {
        // Add to buffer
        this.messageBuffer.push(message);
        // Limit buffer size
        if (this.messageBuffer.length > this.maxBufferSize) {
            this.messageBuffer.shift();
        }
        // Parse and emit typed MIDI events
        const data = message.data;
        if (data.length > 0) {
            const statusByte = data[0];
            if (statusByte === undefined)
                return;
            const channel = statusByte & 0x0f;
            const messageType = statusByte & 0xf0;
            const parsedMessage = {
                timestamp: message.timestamp,
                data: data,
                type: 'unknown',
                channel,
            };
            switch (messageType) {
                case 0xb0: // Control Change
                    parsedMessage.type = 'controlchange';
                    parsedMessage.controller = data[1] ?? 0;
                    parsedMessage.value = data[2] ?? 0;
                    this.emit('controlchange', parsedMessage);
                    break;
                case 0x90: // Note On
                    parsedMessage.type = 'noteon';
                    parsedMessage.note = data[1] ?? 0;
                    parsedMessage.velocity = data[2] ?? 0;
                    this.emit('noteon', parsedMessage);
                    break;
                case 0x80: // Note Off
                    parsedMessage.type = 'noteoff';
                    parsedMessage.note = data[1] ?? 0;
                    parsedMessage.velocity = data[2] ?? 0;
                    this.emit('noteoff', parsedMessage);
                    break;
                case 0xf0: // System Exclusive
                    if (data[data.length - 1] === 0xf7) {
                        parsedMessage.type = 'sysex';
                        this.emit('sysex', parsedMessage);
                    }
                    break;
            }
            // Always emit the generic message event
            this.emit('message', message);
        }
    }
}
//# sourceMappingURL=MidiInterface.js.map