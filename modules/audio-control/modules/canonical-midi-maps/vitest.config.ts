import { defineConfig } from 'vitest/config';
import { resolve } from 'path';

export default defineConfig({
  test: {
    environment: 'node',
    globals: true,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      reportsDirectory: './coverage',
      thresholds: {
        global: {
          // Phase 2.1: 20% baseline threshold, targeting 80%
          branches: 20,
          functions: 20,
          lines: 20,
          statements: 20,
        },
      },
      exclude: [
        'coverage/**',
        'dist/**',
        'node_modules/**',
        '**/*.d.ts',
        '**/*.config.*',
        '**/index.ts', // Re-export files typically don't need coverage
        'tests/**',
        '**/*.test.ts',
        '**/*.spec.ts',
        'src/tools/generate-plugin-specs.ts', // CLI entry point
      ],
    },
    setupFiles: ['./tests/setup.ts'],
  },
  resolve: {
    alias: {
      '@': resolve(__dirname, './src'),
    },
  },
});