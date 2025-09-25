/**
 * Test setup and global configuration for canonical-midi-maps tests
 */

import { vi, beforeEach } from 'vitest';

// Global test environment setup
beforeEach(() => {
  // Clear all mocks before each test
  vi.clearAllMocks();

  // Reset environment variables
  process.env.JUCE_DISABLE_AUDIOUNIT_PLUGIN_SCANNING = '1';

  // Set up common test directories
  process.env.TEST_OUTPUT_DIR = '/tmp/test-output';
  process.env.TEST_PLUGHOST_PATH = '/tmp/test-plughost';
});

// Global test utilities
export const createMockPluginInfo = (overrides = {}) => ({
  manufacturer: 'Test Manufacturer',
  name: 'Test Plugin',
  version: '1.0.0',
  format: 'VST3',
  uid: 'test-uid-123',
  category: 'Instrument',
  ...overrides,
});

export const createMockPluginParameter = (overrides = {}) => ({
  index: 0,
  name: 'Test Parameter',
  label: 'Test Label',
  text: 'Test Text',
  default_value: 0.5,
  current_value: 0.5,
  automatable: true,
  meta_parameter: false,
  discrete: false,
  ...overrides,
});

export const createMockProcessResult = (overrides = {}) => ({
  stdout: '{}',
  stderr: '',
  exitCode: 0,
  ...overrides,
});

// Common test constants
export const TEST_CONSTANTS = {
  MOCK_PLUGHOST_PATH: '/tmp/test-plughost',
  MOCK_OUTPUT_DIR: '/tmp/test-output',
  TIMEOUT_MS: 5000,
} as const;