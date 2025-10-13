/**
 * Node.js-specific exports for Launch Control XL3
 *
 * This entry point includes Node.js-only backends like NodeMidiBackend
 * that cannot be included in browser bundles.
 */

// Re-export everything from main index
export * from './index.js';

// Node.js-only: Node MIDI Backend (uses @julusian/midi directly)
export { NodeMidiBackend } from './backends/NodeMidiBackend.js';
