/**
 * Unit tests for ParameterMatcher service.
 *
 * Tests fuzzy matching of controller names to plugin parameters using Claude AI.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import {
  ParameterMatcher,
  loadPluginDescriptor,
  type ParameterMatchResult,
} from '../ParameterMatcher.js';
import type { PluginDescriptor } from '@oletizi/canonical-midi-maps';

// Mock plugin descriptor for testing
const mockPluginDescriptor: PluginDescriptor = {
  plugin: {
    manufacturer: 'Test Manufacturer',
    name: 'Test Plugin',
    version: '1.0.0',
    format: 'VST3',
    uid: 'test-uid',
  },
  metadata: {
    version: '1.0',
    created: '2024-01-01T00:00:00Z',
    updated: '2024-01-01T00:00:00Z',
    author: 'test',
    parameter_count: 5,
  },
  parameters: [
    {
      index: 0,
      name: 'Master Volume',
      min: 0,
      max: 1,
      default: 0.5,
      group: 'mixer',
      type: 'continuous',
      automatable: true,
    },
    {
      index: 1,
      name: 'Filter Cutoff',
      min: 0,
      max: 1,
      default: 0.5,
      group: 'filter',
      type: 'continuous',
      automatable: true,
    },
    {
      index: 2,
      name: 'Filter Resonance',
      min: 0,
      max: 1,
      default: 0.3,
      group: 'filter',
      type: 'continuous',
      automatable: true,
    },
    {
      index: 3,
      name: 'Envelope Attack',
      min: 0,
      max: 1,
      default: 0.1,
      group: 'envelope',
      type: 'continuous',
      automatable: true,
    },
    {
      index: 4,
      name: 'LFO Rate',
      min: 0,
      max: 1,
      default: 0.5,
      group: 'modulation',
      type: 'continuous',
      automatable: true,
    },
  ],
};

describe('ParameterMatcher', () => {
  let matcher: ParameterMatcher;

  beforeEach(() => {
    matcher = ParameterMatcher.create();
  });

  describe('constructor and factory', () => {
    it('should create instance with default options', () => {
      const instance = ParameterMatcher.create();
      expect(instance).toBeInstanceOf(ParameterMatcher);
    });

    it('should create instance with custom options', () => {
      const instance = ParameterMatcher.create({
        minConfidence: 0.7,
        timeout: 60000,
      });
      expect(instance).toBeInstanceOf(ParameterMatcher);
    });
  });

  describe('matchParameters', () => {
    it('should handle successful Claude API response', async () => {
      // Mock fetch for API call
      const mockResponse = JSON.stringify([
        {
          controlName: 'Cutoff',
          parameterIndex: 1,
          parameterName: 'Filter Cutoff',
          confidence: 0.95,
          reasoning: 'Direct semantic match',
        },
        {
          controlName: 'Resonance',
          parameterIndex: 2,
          parameterName: 'Filter Resonance',
          confidence: 0.9,
          reasoning: 'Direct semantic match',
        },
      ]);

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      // Set API key to trigger API path
      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(
        ['Cutoff', 'Resonance'],
        mockPluginDescriptor
      );

      expect(results).toHaveLength(2);
      expect(results[0].controlName).toBe('Cutoff');
      expect(results[0].pluginParameter).toBe(1);
      expect(results[0].confidence).toBe(0.95);
      expect(results[1].controlName).toBe('Resonance');
      expect(results[1].pluginParameter).toBe(2);

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should filter results below minimum confidence', async () => {
      const mockResponse = JSON.stringify([
        {
          controlName: 'Unknown',
          parameterIndex: null,
          parameterName: null,
          confidence: 0.2,
          reasoning: 'No good match found',
        },
      ]);

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(
        ['Unknown'],
        mockPluginDescriptor,
        { minConfidence: 0.5 }
      );

      expect(results).toHaveLength(1);
      expect(results[0].pluginParameter).toBeUndefined();
      expect(results[0].confidence).toBe(0.2);

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle Claude API errors gracefully', async () => {
      global.fetch = vi.fn().mockResolvedValue({
        ok: false,
        status: 500,
        text: async () => 'Internal server error',
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await expect(
        matcher.matchParameters(['Test'], mockPluginDescriptor)
      ).rejects.toThrow('Claude API request failed');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle empty API response', async () => {
      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await expect(
        matcher.matchParameters(['Test'], mockPluginDescriptor)
      ).rejects.toThrow('Claude API returned empty response');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle invalid JSON in Claude response', async () => {
      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: 'Not valid JSON' }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await expect(
        matcher.matchParameters(['Test'], mockPluginDescriptor)
      ).rejects.toThrow('Could not find JSON array in Claude response');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle malformed JSON in Claude response', async () => {
      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: '[{invalid json}]' }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await expect(
        matcher.matchParameters(['Test'], mockPluginDescriptor)
      ).rejects.toThrow('Failed to parse Claude response as JSON');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle non-array JSON response', async () => {
      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: '{"not": "an array"}' }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await expect(
        matcher.matchParameters(['Test'], mockPluginDescriptor)
      ).rejects.toThrow('Claude response is not an array');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should validate parameter indices from Claude', async () => {
      const mockResponse = JSON.stringify([
        {
          controlName: 'Test',
          parameterIndex: 999, // Invalid index
          parameterName: 'Invalid',
          confidence: 0.9,
          reasoning: 'Test',
        },
      ]);

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(['Test'], mockPluginDescriptor);

      expect(results).toHaveLength(1);
      expect(results[0].pluginParameter).toBeUndefined();
      expect(results[0].confidence).toBe(0);
      expect(results[0].reasoning).toContain('Invalid parameter index');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle null parameter indices from Claude', async () => {
      const mockResponse = JSON.stringify([
        {
          controlName: 'Test',
          parameterIndex: null,
          parameterName: null,
          confidence: 0.1,
          reasoning: 'No match',
        },
      ]);

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(['Test'], mockPluginDescriptor);

      expect(results).toHaveLength(1);
      expect(results[0].pluginParameter).toBeUndefined();
      expect(results[0].confidence).toBe(0.1);

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should handle missing matches in Claude response', async () => {
      const mockResponse = JSON.stringify([
        {
          controlName: 'Control1',
          parameterIndex: 0,
          parameterName: 'Master Volume',
          confidence: 0.8,
          reasoning: 'Match',
        },
        // Missing second control
      ]);

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(
        ['Control1', 'Control2'],
        mockPluginDescriptor
      );

      expect(results).toHaveLength(2);
      expect(results[0].pluginParameter).toBe(0);
      expect(results[1].pluginParameter).toBeUndefined();
      expect(results[1].reasoning).toBe('No match found');

      delete process.env.ANTHROPIC_API_KEY;
    });

    it('should extract JSON from markdown code blocks', async () => {
      const mockResponse = '```json\n' + JSON.stringify([
        {
          controlName: 'Test',
          parameterIndex: 0,
          parameterName: 'Master Volume',
          confidence: 0.8,
          reasoning: 'Match',
        },
      ]) + '\n```';

      global.fetch = vi.fn().mockResolvedValue({
        ok: true,
        json: async () => ({
          content: [{ type: 'text', text: mockResponse }],
        }),
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      const results = await matcher.matchParameters(['Test'], mockPluginDescriptor);

      expect(results).toHaveLength(1);
      expect(results[0].pluginParameter).toBe(0);

      delete process.env.ANTHROPIC_API_KEY;
    });
  });

  describe('buildMatchingPrompt', () => {
    it('should include plugin and parameter information', async () => {
      // We can't directly test private methods, but we can test the effect
      // by ensuring the API receives a properly formatted prompt
      let capturedPrompt = '';

      global.fetch = vi.fn().mockImplementation(async (url, options) => {
        const body = JSON.parse(options?.body as string);
        capturedPrompt = body.messages[0].content;

        return {
          ok: true,
          json: async () => ({
            content: [{ type: 'text', text: '[]' }],
          }),
        };
      });

      process.env.ANTHROPIC_API_KEY = 'test-key';

      await matcher.matchParameters(['Cutoff'], mockPluginDescriptor);

      expect(capturedPrompt).toContain('Test Manufacturer');
      expect(capturedPrompt).toContain('Test Plugin');
      expect(capturedPrompt).toContain('Filter Cutoff');
      expect(capturedPrompt).toContain('Cutoff');

      delete process.env.ANTHROPIC_API_KEY;
    });
  });
});

describe('loadPluginDescriptor', () => {
  it('should throw error if plugin descriptor not found', async () => {
    await expect(loadPluginDescriptor('nonexistent-plugin')).rejects.toThrow(
      'Plugin descriptor not found'
    );
  });

  it('should normalize plugin names to lowercase with hyphens', async () => {
    // This test depends on actual filesystem, but we're testing the error message
    // which shows the normalized name
    try {
      await loadPluginDescriptor('Test Plugin Name');
    } catch (error) {
      expect(error).toBeInstanceOf(Error);
      expect((error as Error).message).toContain('test-plugin-name');
    }
  });

  // Note: Testing actual file loading would require either:
  // 1. Mocking fs.readFile and fs.readdir (complex with ESM)
  // 2. Having test fixture files in place
  // 3. Integration tests with real plugin descriptors
  // For unit tests, we focus on error paths and validation logic
});
