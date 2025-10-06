/**
 * Node.js-specific exports for Launch Control XL3
 *
 * This entry point includes Node.js-only backends like JuceMidiBackend
 * that cannot be included in browser bundles.
 */

// Re-export everything from main index
export * from './index.js';

// Node.js-only: JUCE MIDI Backend
export { JuceMidiBackend } from './backends/JuceMidiBackend.js';
