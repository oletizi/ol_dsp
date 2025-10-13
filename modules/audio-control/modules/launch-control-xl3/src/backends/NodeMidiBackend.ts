/**
 * Node.js MIDI Backend - Direct @julusian/midi integration
 *
 * This backend uses @julusian/midi directly for MIDI operations.
 * Designed for single input/output port usage to avoid multi-port crashes.
 * Ideal for SysEx-based protocols that only need one bidirectional connection.
 */

import { Input, Output } from '@julusian/midi';
import { EventEmitter } from 'events';
import {
  MidiBackendInterface,
  MidiPortInfo,
  MidiInputPort,
  MidiOutputPort,
  MidiMessage,
  MidiPort,
} from '../core/MidiInterface.js';

/**
 * Node MIDI input port wrapper
 */
class NodeMidiInputPort implements MidiInputPort {
  readonly type = 'input' as const;
  onMessage?: ((message: MidiMessage) => void) | undefined;

  constructor(
    readonly id: string,
    readonly name: string,
    private backend: NodeMidiBackend
  ) {}

  async close(): Promise<void> {
    await this.backend.closePort(this);
  }
}

/**
 * Node MIDI output port wrapper
 */
class NodeMidiOutputPort implements MidiOutputPort {
  readonly type = 'output' as const;

  constructor(
    readonly id: string,
    readonly name: string,
    private backend: NodeMidiBackend
  ) {}

  async close(): Promise<void> {
    await this.backend.closePort(this);
  }
}

/**
 * Node.js MIDI Backend using @julusian/midi
 *
 * Constraints:
 * - Opens only ONE input and ONE output port to avoid multi-port crashes
 * - Properly handles SysEx messages
 * - Event-driven message reception
 */
export class NodeMidiBackend extends EventEmitter implements MidiBackendInterface {
  private inputDevice: Input | null = null;
  private outputDevice: Output | null = null;
  private openInputPort: NodeMidiInputPort | null = null;
  private openOutputPort: NodeMidiOutputPort | null = null;
  private isInitialized = false;

  constructor() {
    super();
  }

  async initialize(): Promise<void> {
    if (this.isInitialized) {
      return;
    }

    try {
      // Verify @julusian/midi is available by creating test instances
      const testInput = new Input();
      testInput.closePort();

      const testOutput = new Output();
      testOutput.closePort();

      this.isInitialized = true;
    } catch (error: unknown) {
      const message = error instanceof Error ? error.message : String(error);
      throw new Error(`Failed to initialize @julusian/midi: ${message}`);
    }
  }

  async getInputPorts(): Promise<MidiPortInfo[]> {
    if (!this.isInitialized) {
      throw new Error('NodeMidiBackend not initialized');
    }

    const device = new Input();
    try {
      const portCount = device.getPortCount();
      const ports: MidiPortInfo[] = [];

      for (let i = 0; i < portCount; i++) {
        const name = device.getPortName(i);
        ports.push({
          id: String(i),
          name,
        });
      }

      return ports;
    } finally {
      device.closePort();
    }
  }

  async getOutputPorts(): Promise<MidiPortInfo[]> {
    if (!this.isInitialized) {
      throw new Error('NodeMidiBackend not initialized');
    }

    const device = new Output();
    try {
      const portCount = device.getPortCount();
      const ports: MidiPortInfo[] = [];

      for (let i = 0; i < portCount; i++) {
        const name = device.getPortName(i);
        ports.push({
          id: String(i),
          name,
        });
      }

      return ports;
    } finally {
      device.closePort();
    }
  }

  async openInput(portId: string): Promise<MidiInputPort> {
    if (!this.isInitialized) {
      throw new Error('NodeMidiBackend not initialized');
    }

    // Enforce single port constraint
    if (this.openInputPort) {
      throw new Error(
        'NodeMidiBackend only supports one open input port at a time. Close existing port first.'
      );
    }

    const portIndex = parseInt(portId, 10);
    if (isNaN(portIndex)) {
      throw new Error(`Invalid port ID: ${portId} (must be numeric index)`);
    }

    try {
      this.inputDevice = new Input();
      const portName = this.inputDevice.getPortName(portIndex);

      // Enable SysEx support
      this.inputDevice.ignoreTypes(false, false, false);

      // Set up message handler
      this.inputDevice.on('message', (_deltaTime: number, messageData: number[]) => {
        const message: MidiMessage = {
          timestamp: Date.now(),
          data: messageData,
        };

        // Call the port's onMessage callback if set
        if (this.openInputPort?.onMessage) {
          this.openInputPort.onMessage(message);
        }

        // Emit for backend-level listeners
        this.emit('message', this.openInputPort, message);
      });

      // Open the port
      this.inputDevice.openPort(portIndex);

      this.openInputPort = new NodeMidiInputPort(portId, portName, this);
      return this.openInputPort;
    } catch (error: unknown) {
      // Clean up on error
      if (this.inputDevice) {
        this.inputDevice.closePort();
        this.inputDevice = null;
      }
      const message = error instanceof Error ? error.message : String(error);
      throw new Error(`Failed to open MIDI input port ${portId}: ${message}`);
    }
  }

  async openOutput(portId: string): Promise<MidiOutputPort> {
    if (!this.isInitialized) {
      throw new Error('NodeMidiBackend not initialized');
    }

    // Enforce single port constraint
    if (this.openOutputPort) {
      throw new Error(
        'NodeMidiBackend only supports one open output port at a time. Close existing port first.'
      );
    }

    const portIndex = parseInt(portId, 10);
    if (isNaN(portIndex)) {
      throw new Error(`Invalid port ID: ${portId} (must be numeric index)`);
    }

    try {
      this.outputDevice = new Output();
      const portName = this.outputDevice.getPortName(portIndex);

      // Open the port
      this.outputDevice.openPort(portIndex);

      this.openOutputPort = new NodeMidiOutputPort(portId, portName, this);
      return this.openOutputPort;
    } catch (error: unknown) {
      // Clean up on error
      if (this.outputDevice) {
        this.outputDevice.closePort();
        this.outputDevice = null;
      }
      const message = error instanceof Error ? error.message : String(error);
      throw new Error(`Failed to open MIDI output port ${portId}: ${message}`);
    }
  }

  async sendMessage(port: MidiOutputPort, message: MidiMessage): Promise<void> {
    if (!this.outputDevice) {
      throw new Error('No output device available');
    }

    if (port !== this.openOutputPort) {
      throw new Error('Port is not open');
    }

    try {
      // Convert readonly array to mutable array for @julusian/midi
      const messageData = Array.from(message.data);
      this.outputDevice.sendMessage(messageData);
    } catch (error: unknown) {
      const message_text = error instanceof Error ? error.message : String(error);
      throw new Error(`Failed to send MIDI message: ${message_text}`);
    }
  }

  async closePort(port: MidiPort): Promise<void> {
    if (port.type === 'input') {
      if (port !== this.openInputPort) {
        throw new Error('Input port is not open');
      }

      if (this.inputDevice) {
        this.inputDevice.removeAllListeners('message');
        this.inputDevice.closePort();
        this.inputDevice = null;
      }

      this.openInputPort = null;
    } else {
      if (port !== this.openOutputPort) {
        throw new Error('Output port is not open');
      }

      if (this.outputDevice) {
        this.outputDevice.closePort();
        this.outputDevice = null;
      }

      this.openOutputPort = null;
    }
  }

  async cleanup(): Promise<void> {
    // Close input port if open
    if (this.openInputPort) {
      await this.closePort(this.openInputPort);
    }

    // Close output port if open
    if (this.openOutputPort) {
      await this.closePort(this.openOutputPort);
    }

    this.removeAllListeners();
    this.isInitialized = false;
  }
}
