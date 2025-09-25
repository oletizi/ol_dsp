/**
 * Test setup and global configuration for ardour-midi-maps tests
 */

import { vi, beforeEach } from 'vitest';

// Global test environment setup
beforeEach(() => {
  // Clear all mocks before each test
  vi.clearAllMocks();

  // Set up common test directories
  process.env.TEST_OUTPUT_DIR = '/tmp/test-ardour-output';
});

// Global test utilities for Ardour MIDI testing
export const createMockMidiBinding = (overrides = {}) => ({
  channel: 1,
  cc: 1,
  parameter: 'gain',
  min: 0,
  max: 1,
  ...overrides,
});

export const createMockControllerDefinition = (overrides = {}) => ({
  name: 'Test Controller',
  manufacturer: 'Test Manufacturer',
  bindings: [],
  ...overrides,
});

// Common test constants
export const TEST_CONSTANTS = {
  MOCK_OUTPUT_DIR: '/tmp/test-ardour-output',
  DEFAULT_CHANNEL: 1,
  DEFAULT_CC: 1,
} as const;