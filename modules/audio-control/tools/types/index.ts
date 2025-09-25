/**
 * Audio Control Tools - Type Definitions
 *
 * Consolidated TypeScript interfaces for the 12-tool workflow.
 * Provides clean data contracts for tool communication and data exchange.
 */

// Re-export all types for easy access
export * from './plugin.js';
export * from './midi.js';
export * from './daw.js';
export * from './workflow.js';

// Common utility types
export type Optional<T, K extends keyof T> = Omit<T, K> & Partial<Pick<T, K>>;
export type Required<T, K extends keyof T> = T & Required<Pick<T, K>>;

// File system types
export interface FileSystemResult {
  success: boolean;
  path?: string;
  content?: string;
  error?: string;
  stats?: {
    size: number;
    created: string;
    modified: string;
  };
}

// Configuration types
export interface ToolConfig {
  /** Tool name */
  name: string;

  /** Tool version */
  version: string;

  /** Tool options */
  options?: Record<string, any>;

  /** Input/output directories */
  paths?: {
    input?: string;
    output?: string;
    cache?: string;
    logs?: string;
  };

  /** Tool dependencies */
  dependencies?: string[];
}

// Logging types
export type LogLevel = 'debug' | 'info' | 'warn' | 'error';

export interface LogEntry {
  timestamp: string;
  level: LogLevel;
  message: string;
  tool?: string;
  phase?: string;
  data?: any;
}

// Error types
export interface ToolError extends Error {
  code: string;
  tool: string;
  phase?: string;
  data?: any;
  recoverable?: boolean;
}

export class ValidationError extends Error {
  constructor(
    message: string,
    public code: string,
    public path: string,
    public data?: any
  ) {
    super(message);
    this.name = 'ValidationError';
  }
}

export class WorkflowError extends Error {
  constructor(
    message: string,
    public phase: string,
    public tool: string,
    public data?: any
  ) {
    super(message);
    this.name = 'WorkflowError';
  }
}