/**
 * Workflow Data Contracts
 *
 * Core interfaces for workflow results, validation, and tool communication
 * used across all tools in the workflow.
 */

export interface PhaseResult {
  /** Phase identifier */
  phase: string;

  /** Phase success status */
  success: boolean;

  /** Phase start time */
  startTime: string;

  /** Phase end time */
  endTime?: string;

  /** Phase duration in milliseconds */
  duration?: number;

  /** Phase output data */
  data?: any;

  /** Phase errors */
  errors?: string[];

  /** Phase warnings */
  warnings?: string[];

  /** Next phase to execute */
  nextPhase?: string;

  /** Phase metadata */
  metadata?: Record<string, any>;
}

export interface ValidationResult {
  /** Validation success */
  valid: boolean;

  /** Validation errors */
  errors: ValidationError[];

  /** Validation warnings */
  warnings: ValidationWarning[];

  /** Validation score (0-1) */
  score?: number;

  /** Validation metadata */
  metadata?: ValidationMetadata;
}

export interface ValidationError {
  /** Error path/location */
  path: string;

  /** Error message */
  message: string;

  /** Error code */
  code: string;

  /** Error severity */
  severity?: 'low' | 'medium' | 'high' | 'critical';

  /** Suggested fix */
  fix?: string;
}

export interface ValidationWarning {
  /** Warning path/location */
  path: string;

  /** Warning message */
  message: string;

  /** Warning code */
  code: string;

  /** Warning category */
  category?: string;
}

export interface ValidationMetadata {
  /** Validation timestamp */
  timestamp: string;

  /** Validator version */
  validator: string;

  /** Validation rules applied */
  rules?: string[];

  /** Validation duration */
  duration?: number;
}

export interface WorkflowConfig {
  /** Workflow identifier */
  id: string;

  /** Workflow name */
  name: string;

  /** Workflow phases */
  phases: WorkflowPhase[];

  /** Global workflow options */
  options?: WorkflowOptions;
}

export interface WorkflowPhase {
  /** Phase identifier */
  id: string;

  /** Phase name */
  name: string;

  /** Phase description */
  description?: string;

  /** Tool/script to execute */
  tool: string;

  /** Phase dependencies */
  dependencies?: string[];

  /** Phase options */
  options?: Record<string, any>;

  /** Skip conditions */
  skipIf?: string[];

  /** Retry configuration */
  retry?: RetryConfig;
}

export interface WorkflowOptions {
  /** Parallel execution where possible */
  parallel?: boolean;

  /** Stop on first error */
  stopOnError?: boolean;

  /** Output directory */
  outputDir?: string;

  /** Log level */
  logLevel?: 'debug' | 'info' | 'warn' | 'error';

  /** Cleanup on failure */
  cleanupOnFailure?: boolean;
}

export interface RetryConfig {
  /** Maximum retry attempts */
  maxAttempts: number;

  /** Delay between retries (ms) */
  delay: number;

  /** Backoff strategy */
  backoff?: 'none' | 'linear' | 'exponential';
}

export interface WorkflowExecution {
  /** Execution identifier */
  id: string;

  /** Workflow configuration */
  config: WorkflowConfig;

  /** Execution start time */
  startTime: string;

  /** Execution end time */
  endTime?: string;

  /** Execution status */
  status: 'running' | 'completed' | 'failed' | 'cancelled';

  /** Phase results */
  phases: PhaseResult[];

  /** Overall success */
  success?: boolean;

  /** Total duration */
  duration?: number;

  /** Execution metadata */
  metadata?: Record<string, any>;
}

export interface ToolExecutionContext {
  /** Tool name */
  toolName: string;

  /** Execution ID */
  executionId: string;

  /** Phase ID */
  phaseId: string;

  /** Input data */
  input?: any;

  /** Tool options */
  options?: Record<string, any>;

  /** Working directory */
  workingDir: string;

  /** Output directory */
  outputDir: string;

  /** Shared data between tools */
  sharedData?: Record<string, any>;
}

export interface ToolExecutionResult {
  /** Execution success */
  success: boolean;

  /** Output data */
  output?: any;

  /** Execution errors */
  errors?: string[];

  /** Execution warnings */
  warnings?: string[];

  /** Tool metadata */
  metadata?: Record<string, any>;

  /** Files created/modified */
  files?: string[];

  /** Data to share with other tools */
  sharedData?: Record<string, any>;
}

export interface QualityMetrics {
  /** Type coverage percentage */
  typeCoverage?: number;

  /** Test coverage percentage */
  testCoverage?: number;

  /** Build success rate */
  buildSuccess?: boolean;

  /** Linting issues count */
  lintIssues?: number;

  /** Performance metrics */
  performance?: PerformanceMetrics;

  /** Security issues count */
  securityIssues?: number;
}

export interface PerformanceMetrics {
  /** Build time in milliseconds */
  buildTime?: number;

  /** Bundle size in bytes */
  bundleSize?: number;

  /** Memory usage in MB */
  memoryUsage?: number;

  /** CPU usage percentage */
  cpuUsage?: number;
}