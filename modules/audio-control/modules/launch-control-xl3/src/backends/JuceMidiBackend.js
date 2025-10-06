/**
 * JUCE MIDI Backend - Proxies MIDI through external JUCE server
 *
 * This backend communicates with a JUCE-based MIDI server to handle
 * all MIDI operations, avoiding the limitations of Node.js MIDI libraries.
 */
import { EventEmitter } from 'events';
class JuceMidiInputPort {
    id;
    name;
    backend;
    type = 'input';
    onMessage;
    constructor(id, name, backend) {
        this.id = id;
        this.name = name;
        this.backend = backend;
    }
    async close() {
        await this.backend.closePort(this);
    }
}
class JuceMidiOutputPort {
    id;
    name;
    backend;
    type = 'output';
    constructor(id, name, backend) {
        this.id = id;
        this.name = name;
        this.backend = backend;
    }
    async close() {
        await this.backend.closePort(this);
    }
}
export class JuceMidiBackend extends EventEmitter {
    config;
    pollInterval = null;
    openPorts = new Map(); // portId -> MidiPort
    constructor(config = { host: 'localhost', port: 7777 }) {
        super();
        this.config = config;
    }
    get baseUrl() {
        return `http://${this.config.host}:${this.config.port}`;
    }
    async initialize() {
        // Check if server is running
        try {
            const response = await fetch(`${this.baseUrl}/health`);
            const data = await response.json();
            if (data.status !== 'ok') {
                throw new Error('JUCE MIDI server not healthy');
            }
        }
        catch (error) {
            throw new Error(`Failed to connect to JUCE MIDI server at ${this.baseUrl}: ${error.message}`);
        }
    }
    async getInputPorts() {
        try {
            const response = await fetch(`${this.baseUrl}/ports`);
            const data = await response.json();
            return data.inputs.map((name) => ({
                id: name,
                name
            }));
        }
        catch (error) {
            console.error('Failed to get MIDI inputs:', error);
            return [];
        }
    }
    async getOutputPorts() {
        try {
            const response = await fetch(`${this.baseUrl}/ports`);
            const data = await response.json();
            return data.outputs.map((name) => ({
                id: name,
                name
            }));
        }
        catch (error) {
            console.error('Failed to get MIDI outputs:', error);
            return [];
        }
    }
    async openInput(portId) {
        try {
            const response = await fetch(`${this.baseUrl}/port/${portId}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: portId,
                    type: 'input'
                })
            });
            const data = await response.json();
            if (!data.success) {
                throw new Error(`Failed to open input port: ${portId}`);
            }
            const port = new JuceMidiInputPort(portId, portId, this);
            this.openPorts.set(portId, port);
            // Start polling for messages
            this.startPolling();
            return port;
        }
        catch (error) {
            throw new Error(`Failed to open MIDI input ${portId}: ${error.message}`);
        }
    }
    async openOutput(portId) {
        try {
            const response = await fetch(`${this.baseUrl}/port/${portId}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    name: portId,
                    type: 'output'
                })
            });
            const data = await response.json();
            if (!data.success) {
                throw new Error(`Failed to open output port: ${portId}`);
            }
            const port = new JuceMidiOutputPort(portId, portId, this);
            this.openPorts.set(portId, port);
            return port;
        }
        catch (error) {
            throw new Error(`Failed to open MIDI output ${portId}: ${error.message}`);
        }
    }
    async sendMessage(port, message) {
        try {
            await fetch(`${this.baseUrl}/port/${port.id}/send`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    message: Array.from(message.data)
                })
            });
        }
        catch (error) {
            throw new Error(`Failed to send MIDI message: ${error.message}`);
        }
    }
    async closePort(port) {
        try {
            await fetch(`${this.baseUrl}/port/${port.id}`, { method: 'DELETE' });
            this.openPorts.delete(port.id);
        }
        catch (error) {
            throw new Error(`Failed to close port ${port.id}: ${error.message}`);
        }
    }
    async cleanup() {
        this.stopPolling();
        // Close all open ports
        for (const port of Array.from(this.openPorts.values())) {
            try {
                await this.closePort(port);
            }
            catch (error) {
                console.error(`Error closing port ${port.id}:`, error);
            }
        }
        this.openPorts.clear();
        this.removeAllListeners();
    }
    startPolling() {
        if (this.pollInterval)
            return;
        this.pollInterval = setInterval(async () => {
            // Poll all open input ports
            for (const [portId, port] of Array.from(this.openPorts.entries())) {
                if (port.type === 'input') {
                    try {
                        const response = await fetch(`${this.baseUrl}/port/${portId}/messages?timeout=0`);
                        const data = await response.json();
                        if (data.messages && data.messages.length > 0) {
                            for (const messageData of data.messages) {
                                const message = {
                                    data: Array.from(messageData),
                                    timestamp: Date.now()
                                };
                                // Call the onMessage callback if set
                                const inputPort = port;
                                if (inputPort.onMessage) {
                                    inputPort.onMessage(message);
                                }
                                // Emit for backend-level listeners
                                this.emit('message', port, message);
                            }
                        }
                    }
                    catch (error) {
                        console.error(`Error polling MIDI input ${portId}:`, error);
                    }
                }
            }
            // Poll DAW input if open
            if (this.openPorts.has('daw_in')) {
                try {
                    const response = await fetch(`${this.baseUrl}/port/daw_in/messages?timeout=0`);
                    const data = await response.json();
                    if (data.messages && data.messages.length > 0) {
                        for (const messageData of data.messages) {
                            const message = {
                                data: Array.from(messageData),
                                timestamp: Date.now()
                            };
                            this.emit('dawMessage', message);
                        }
                    }
                }
                catch (error) {
                    console.error('Error polling DAW input:', error);
                }
            }
        }, 10); // Poll every 10ms for low latency
    }
    stopPolling() {
        if (this.pollInterval) {
            clearInterval(this.pollInterval);
            this.pollInterval = null;
        }
    }
    async close() {
        this.stopPolling();
        // Close all open ports
        for (const [portId, _] of Array.from(this.openPorts.entries())) {
            try {
                await fetch(`${this.baseUrl}/port/${portId}`, { method: 'DELETE' });
            }
            catch (error) {
                console.error(`Error closing port ${portId}:`, error);
            }
        }
        this.openPorts.clear();
        this.removeAllListeners();
    }
    dispose() {
        this.close().catch(console.error);
    }
}
//# sourceMappingURL=JuceMidiBackend.js.map