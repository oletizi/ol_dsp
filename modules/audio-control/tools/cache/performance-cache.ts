#!/usr/bin/env tsx

/**
 * Performance Cache Module
 * Implements caching strategies to meet performance targets
 */

import { readFile, writeFile, stat, mkdir } from 'fs/promises';
import { existsSync } from 'fs';
import { resolve, dirname } from 'path';
import { createHash } from 'crypto';

export interface CacheOptions {
  /** Cache directory path */
  cacheDir?: string;
  /** Cache TTL in milliseconds */
  ttl?: number;
  /** Enable compression */
  compress?: boolean;
}

export interface CacheEntry<T = any> {
  data: T;
  timestamp: number;
  hash: string;
  metadata?: Record<string, any>;
}

/**
 * High-performance cache for tool operations
 * Targets: <10ms validation overhead, <2s end-to-end workflow
 */
export class PerformanceCache {
  private readonly cacheDir: string;
  private readonly ttl: number;
  private readonly compress: boolean;

  constructor(options: CacheOptions = {}) {
    this.cacheDir = options.cacheDir || resolve(process.cwd(), '.cache');
    this.ttl = options.ttl || 24 * 60 * 60 * 1000; // 24 hours
    this.compress = options.compress || false;
  }

  /**
   * Initialize cache directory
   */
  async init(): Promise<void> {
    if (!existsSync(this.cacheDir)) {
      await mkdir(this.cacheDir, { recursive: true });
    }
  }

  /**
   * Generate cache key from input data
   */
  private generateKey(namespace: string, input: any): string {
    const hash = createHash('sha256');
    hash.update(JSON.stringify(input));
    return `${namespace}_${hash.digest('hex').substring(0, 16)}`;
  }

  /**
   * Get cached data with performance optimization
   */
  async get<T>(namespace: string, input: any): Promise<T | null> {
    const startTime = performance.now();

    try {
      const key = this.generateKey(namespace, input);
      const cachePath = resolve(this.cacheDir, `${key}.json`);

      if (!existsSync(cachePath)) {
        return null;
      }

      // Check if cache is expired
      const stats = await stat(cachePath);
      const age = Date.now() - stats.mtime.getTime();

      if (age > this.ttl) {
        return null;
      }

      const content = await readFile(cachePath, 'utf-8');
      const entry: CacheEntry<T> = JSON.parse(content);

      const duration = performance.now() - startTime;

      // Log if cache read exceeds performance target
      if (duration > 10) {
        console.warn(`Cache read took ${duration.toFixed(1)}ms (target: <10ms)`);
      }

      return entry.data;

    } catch (error) {
      // Cache miss or error - return null
      return null;
    }
  }

  /**
   * Set cached data with metadata
   */
  async set<T>(namespace: string, input: any, data: T, metadata?: Record<string, any>): Promise<void> {
    const startTime = performance.now();

    try {
      await this.init();

      const key = this.generateKey(namespace, input);
      const cachePath = resolve(this.cacheDir, `${key}.json`);

      const entry: CacheEntry<T> = {
        data,
        timestamp: Date.now(),
        hash: this.generateKey('data', data),
        metadata
      };

      await writeFile(cachePath, JSON.stringify(entry, null, 2), 'utf-8');

      const duration = performance.now() - startTime;

      // Log if cache write exceeds performance target
      if (duration > 10) {
        console.warn(`Cache write took ${duration.toFixed(1)}ms (target: <10ms)`);
      }

    } catch (error) {
      console.warn(`Cache write failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Clear cache for namespace or entire cache
   */
  async clear(namespace?: string): Promise<void> {
    if (!namespace) {
      // Clear entire cache directory
      if (existsSync(this.cacheDir)) {
        const { rm } = await import('fs/promises');
        await rm(this.cacheDir, { recursive: true, force: true });
      }
      return;
    }

    // Clear specific namespace
    const { readdir, unlink } = await import('fs/promises');

    try {
      const files = await readdir(this.cacheDir);
      const namespaceFiles = files.filter(f => f.startsWith(`${namespace}_`));

      await Promise.all(
        namespaceFiles.map(f =>
          unlink(resolve(this.cacheDir, f)).catch(() => {}) // Ignore errors
        )
      );
    } catch (error) {
      // Directory might not exist - ignore
    }
  }

  /**
   * Get cache statistics
   */
  async getStats(): Promise<{
    totalFiles: number;
    totalSize: number;
    oldestEntry: Date | null;
    newestEntry: Date | null;
  }> {
    try {
      if (!existsSync(this.cacheDir)) {
        return { totalFiles: 0, totalSize: 0, oldestEntry: null, newestEntry: null };
      }

      const { readdir } = await import('fs/promises');
      const files = await readdir(this.cacheDir);

      let totalSize = 0;
      let oldestEntry: Date | null = null;
      let newestEntry: Date | null = null;

      for (const file of files) {
        const filePath = resolve(this.cacheDir, file);
        const stats = await stat(filePath);

        totalSize += stats.size;

        if (!oldestEntry || stats.mtime < oldestEntry) {
          oldestEntry = stats.mtime;
        }

        if (!newestEntry || stats.mtime > newestEntry) {
          newestEntry = stats.mtime;
        }
      }

      return {
        totalFiles: files.length,
        totalSize,
        oldestEntry,
        newestEntry
      };

    } catch (error) {
      return { totalFiles: 0, totalSize: 0, oldestEntry: null, newestEntry: null };
    }
  }
}

/**
 * Cached function wrapper for performance optimization
 */
export function cached<TArgs extends any[], TReturn>(
  namespace: string,
  fn: (...args: TArgs) => Promise<TReturn>,
  cache?: PerformanceCache
): (...args: TArgs) => Promise<TReturn> {
  const performanceCache = cache || new PerformanceCache();

  return async (...args: TArgs): Promise<TReturn> => {
    // Try to get from cache first
    const cached = await performanceCache.get<TReturn>(namespace, args);
    if (cached !== null) {
      return cached;
    }

    // Execute function and cache result
    const result = await fn(...args);
    await performanceCache.set(namespace, args, result);

    return result;
  };
}

/**
 * Memoization for synchronous functions
 */
export function memoized<TArgs extends any[], TReturn>(
  fn: (...args: TArgs) => TReturn
): (...args: TArgs) => TReturn {
  const cache = new Map<string, TReturn>();

  return (...args: TArgs): TReturn => {
    const key = JSON.stringify(args);

    if (cache.has(key)) {
      return cache.get(key)!;
    }

    const result = fn(...args);
    cache.set(key, result);

    return result;
  };
}

// Singleton cache instance for global use
export const globalCache = new PerformanceCache({
  cacheDir: resolve(process.cwd(), '.cache/audio-control'),
  ttl: 24 * 60 * 60 * 1000, // 24 hours
  compress: false
});