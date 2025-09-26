/**
 * Plugin Data Contracts
 *
 * Core interfaces for plugin information, parameters, and metadata
 * used across all tools in the workflow.
 */

export interface PluginInfo {
  /** Unique plugin identifier */
  id: string;

  /** Plugin display name */
  name: string;

  /** Plugin manufacturer/vendor */
  manufacturer: string;

  /** Plugin version */
  version: string;

  /** Plugin format (VST3, AU, VST, etc.) */
  format: string;

  /** Plugin file path */
  path: string;

  /** Plugin unique identifier (if available) */
  uid?: string;

  /** Plugin category/type */
  category?: string;

  /** Plugin description */
  description?: string;

  /** Number of parameters */
  parameterCount: number;

  /** Last scan timestamp */
  lastScanned?: string;

  /** Scan source (juce-host, reaper-scan, etc.) */
  source?: string;
}

export interface PluginParameter {
  /** Parameter index in the plugin */
  index: number;

  /** Parameter name */
  name: string;

  /** Parameter label/units */
  label?: string;

  /** Minimum value */
  min: number;

  /** Maximum value */
  max: number;

  /** Default value */
  default: number;

  /** Current value (if known) */
  current?: number;

  /** Parameter group/category */
  group?: string;

  /** Parameter description */
  description?: string;

  /** Whether parameter is automatable */
  automatable?: boolean;

  /** Parameter type */
  type?: 'continuous' | 'discrete' | 'boolean' | 'choice';

  /** For choice parameters, available options */
  choices?: string[];

  /** Units of measurement */
  units?: string;
}

export interface PluginParameterMap {
  /** Plugin identifier */
  pluginId: string;

  /** Plugin name for reference */
  pluginName: string;

  /** All plugin parameters */
  parameters: PluginParameter[];

  /** Parameter groups for organization */
  groups?: Record<string, {
    name: string;
    description?: string;
    parameterIndices: number[];
  }>;

  /** Metadata about this parameter mapping */
  metadata: {
    version: string;
    created: string;
    updated: string;
    author: string;
  };
}

export interface PluginScanResult {
  /** Scan timestamp */
  timestamp: string;

  /** Scanner identifier */
  scanner: string;

  /** Scan success status */
  success: boolean;

  /** Discovered plugins */
  plugins: PluginInfo[];

  /** Scan errors (if any) */
  errors?: string[];

  /** Scan warnings (if any) */
  warnings?: string[];

  /** Scan duration in milliseconds */
  duration?: number;
}