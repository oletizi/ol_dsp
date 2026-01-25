import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig({
  plugins: [react()],
  // Use relative paths for assets so app works at any base path
  base: './',
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  server: {
    port: 3330,
    host: true,
    allowedHosts: ['orion-m1'],
  },
  build: {
    outDir: 'dist',
    sourcemap: true,
  },
});
