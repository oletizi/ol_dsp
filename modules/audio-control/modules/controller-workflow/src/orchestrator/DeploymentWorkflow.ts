/**
 * DeploymentWorkflow - Core orchestration engine for controller interrogation, conversion, and deployment
 *
 * Coordinates the complete workflow:
 * 1. Read controller configuration from hardware
 * 2. Convert to canonical MIDI map format
 * 3. Save canonical YAML (optional)
 * 4. Deploy to target DAWs
 *
 * @module controller-workflow/orchestrator
 */

import { EventEmitter } from 'node:events';
import { writeFile, mkdir } from 'node:fs/promises';
import { join } from 'node:path';
import type { CanonicalMidiMap } from '@oletizi/canonical-midi-maps';
import { CanonicalMapParser } from '@oletizi/canonical-midi-maps';
import type {
  ControllerAdapterInterface,
  ControllerConfiguration,
  CanonicalConverterInterface,
  DAWDeployerInterface,
  DeploymentResult,
  ConversionOptions,
} from '@/types/index.js';
import { LaunchControlXL3Adapter } from '@/adapters/controllers/LaunchControlXL3Adapter.js';
import { LaunchControlXL3Converter } from '@/converters/LaunchControlXL3Converter.js';
import type { PluginDefinition } from '@oletizi/canonical-midi-maps';

/**
 * Options for workflow execution
 */
export interface WorkflowOptions {
  /** Configuration slot to read from controller (0-15) */
  configSlot: number;
  /** Target DAW names for deployment (e.g., ['ardour', 'live']) */
  targets: string[];
  /** Optional plugin information to include in canonical map */
  pluginInfo?: PluginDefinition;
  /** Optional MIDI channel override (0-15) */
  midiChannel?: number;
  /** Whether to preserve controller-specific labels */
  preserveLabels?: boolean;
  /** Output directory for canonical YAML and DAW configs */
  outputDir?: string;
  /** Whether to auto-install configs to DAW directories */
  autoInstall?: boolean;
  /** Preview mode - don't write any files */
  dryRun?: boolean;
}

/**
 * Complete workflow execution result
 */
export interface WorkflowResult {
  /** Whether the entire workflow succeeded */
  success: boolean;
  /** Controller configuration read from device */
  controllerConfig?: ControllerConfiguration;
  /** Canonical MIDI map generated from controller config */
  canonicalMap?: CanonicalMidiMap;
  /** Path where canonical YAML was saved */
  canonicalPath?: string;
  /** Results from each DAW deployment */
  deployments: DeploymentResult[];
  /** Accumulated error messages */
  errors: string[];
}

/**
 * Options for creating a DeploymentWorkflow instance
 */
export interface CreateOptions {
  /** Optional pre-configured controller adapter */
  controllerAdapter?: ControllerAdapterInterface;
  /** Target DAW names */
  targets: string[];
  /** Optional pre-configured DAW deployers */
  deployers?: Map<string, DAWDeployerInterface>;
}

/**
 * Progress event emitted during workflow execution
 */
export interface ProgressEvent {
  /** Current step number (1-4) */
  step: number;
  /** Human-readable progress message */
  message: string;
  /** Optional data associated with this step */
  data?: unknown;
}

/**
 * Canonical saved event emitted when YAML is written
 */
export interface CanonicalSavedEvent {
  /** Path where canonical YAML was saved */
  path: string;
  /** The canonical MIDI map that was saved */
  map: CanonicalMidiMap;
}

/**
 * Main orchestrator for controller interrogation and deployment workflow.
 * Extends EventEmitter to provide progress updates throughout the process.
 *
 * @example
 * ```typescript
 * const workflow = await DeploymentWorkflow.create({
 *   targets: ['ardour', 'live']
 * });
 *
 * workflow.on('progress', ({ step, message }) => {
 *   console.log(`Step ${step}: ${message}`);
 * });
 *
 * const result = await workflow.execute({
 *   configSlot: 0,
 *   targets: ['ardour'],
 *   outputDir: './output'
 * });
 * ```
 */
export class DeploymentWorkflow extends EventEmitter {
  private constructor(
    private readonly controllerAdapter: ControllerAdapterInterface,
    private readonly converter: CanonicalConverterInterface,
    private readonly deployers: Map<string, DAWDeployerInterface>,
  ) {
    super();
  }

  /**
   * Create a new DeploymentWorkflow with auto-detection and defaults.
   *
   * @param options - Creation options
   * @returns Configured DeploymentWorkflow instance
   * @throws Error if no controller detected or deployer creation fails
   */
  static async create(options: CreateOptions): Promise<DeploymentWorkflow> {
    // Use provided adapter or auto-detect controller
    const adapter = options.controllerAdapter ?? (await this.detectController());

    // Select appropriate converter for detected controller
    const converter = this.getConverterFor(adapter);

    // Create DAW deployers for requested targets
    const deployers = options.deployers ?? (await this.createDeployers(options.targets));

    return new DeploymentWorkflow(adapter, converter, deployers);
  }

  /**
   * Execute the complete workflow:
   * 1. Read controller configuration
   * 2. Convert to canonical format
   * 3. Save canonical YAML (optional)
   * 4. Deploy to DAWs
   *
   * @param options - Workflow execution options
   * @returns Complete workflow result
   */
  async execute(options: WorkflowOptions): Promise<WorkflowResult> {
    const errors: string[] = [];
    const deployments: DeploymentResult[] = [];
    let controllerConfig: ControllerConfiguration | undefined;
    let canonicalMap: CanonicalMidiMap | undefined;
    let canonicalPath: string | undefined;

    try {
      // Step 1: Read controller configuration
      this.emitProgress(1, 'Reading configuration from controller...');
      controllerConfig = await this.readControllerConfiguration(options.configSlot);
      this.emitProgress(1, `Configuration "${controllerConfig.name}" read successfully`, {
        controlCount: controllerConfig.controls.length,
      });

      // Step 2: Convert to canonical format
      this.emitProgress(2, 'Converting to canonical MIDI map format...');
      canonicalMap = await this.convertToCanonical(controllerConfig, options);
      this.emitProgress(2, `Canonical map created with ${canonicalMap.controls.length} controls`);

      // Step 3: Save canonical YAML (optional)
      if (options.outputDir && !options.dryRun) {
        this.emitProgress(3, 'Saving canonical YAML...');
        canonicalPath = await this.saveCanonicalYAML(canonicalMap, options.outputDir);
        this.emitProgress(3, `Canonical YAML saved to ${canonicalPath}`);
        this.emit('canonical-saved', {
          path: canonicalPath,
          map: canonicalMap,
        } as CanonicalSavedEvent);
      } else if (options.dryRun) {
        this.emitProgress(3, 'Dry run: Skipping canonical YAML save');
      } else {
        this.emitProgress(3, 'No output directory specified, skipping canonical YAML save');
      }

      // Step 4: Deploy to DAWs
      this.emitProgress(4, `Deploying to ${options.targets.length} DAW(s)...`);
      for (const target of options.targets) {
        const deployer = this.deployers.get(target.toLowerCase());
        if (!deployer) {
          const error = `No deployer found for target: ${target}`;
          errors.push(error);
          deployments.push({
            success: false,
            dawName: target,
            errors: [error],
          });
          continue;
        }

        try {
          // Build deployment options with proper type safety
          const deploymentOptions = {
            autoInstall: options.autoInstall ?? undefined,
            outputPath: options.outputDir ? join(options.outputDir, target) : undefined,
            dryRun: options.dryRun ?? undefined,
          };

          const deploymentResult = await deployer.deploy(canonicalMap, deploymentOptions);
          deployments.push(deploymentResult);

          if (deploymentResult.success) {
            this.emitProgress(4, `Successfully deployed to ${deployer.dawName}`);
          } else {
            const deployErrors = deploymentResult.errors ?? ['Unknown deployment error'];
            errors.push(...deployErrors);
            this.emitProgress(4, `Failed to deploy to ${deployer.dawName}: ${deployErrors.join(', ')}`);
          }
        } catch (error) {
          const errorMsg = error instanceof Error ? error.message : 'Unknown deployment error';
          errors.push(errorMsg);
          deployments.push({
            success: false,
            dawName: deployer.dawName,
            errors: [errorMsg],
          });
          this.emitProgress(4, `Deployment error for ${deployer.dawName}: ${errorMsg}`);
        }
      }

      const allDeploymentsSucceeded = deployments.every((d) => d.success);
      if (allDeploymentsSucceeded) {
        this.emitProgress(4, 'All deployments completed successfully');
      } else {
        this.emitProgress(4, `Deployments completed with ${errors.length} error(s)`);
      }

      return {
        success: allDeploymentsSucceeded && errors.length === 0,
        controllerConfig,
        canonicalMap,
        canonicalPath: canonicalPath ?? undefined,
        deployments,
        errors,
      };
    } catch (error) {
      const errorMsg = error instanceof Error ? error.message : 'Unknown workflow error';
      errors.push(errorMsg);
      this.emit('error', new Error(errorMsg));

      return {
        success: false,
        controllerConfig: controllerConfig ?? undefined,
        canonicalMap: canonicalMap ?? undefined,
        canonicalPath: canonicalPath ?? undefined,
        deployments,
        errors,
      };
    }
  }

  // ============================================
  // Private Methods
  // ============================================

  /**
   * Auto-detect connected controller and create appropriate adapter.
   * Currently supports Launch Control XL 3, extensible for future controllers.
   */
  private static async detectController(): Promise<ControllerAdapterInterface> {
    // Try Launch Control XL 3 first
    try {
      const adapter = await LaunchControlXL3Adapter.create();
      if (adapter.isConnected()) {
        return adapter;
      }
    } catch (error) {
      // Controller not found, try next...
    }

    // Future: Add more controller detection here
    // try {
    //   const adapter = await AnotherControllerAdapter.create();
    //   if (adapter.isConnected()) {
    //     return adapter;
    //   }
    // } catch (error) {
    //   // Continue...
    // }

    throw new Error(
      'No supported controller detected. Currently supported: Launch Control XL 3. ' +
        'Ensure your controller is connected and in Custom Mode.',
    );
  }

  /**
   * Select appropriate converter for a controller adapter.
   * Maps controller models to their specific converter implementations.
   */
  private static getConverterFor(adapter: ControllerAdapterInterface): CanonicalConverterInterface {
    const controllerKey = `${adapter.manufacturer}:${adapter.model}`.toLowerCase();

    // Map controller to converter
    const converterMap: Record<string, () => CanonicalConverterInterface> = {
      'novation:launch control xl 3': () => new LaunchControlXL3Converter(),
      // Future: Add more converters here
      // 'akai:mpk mini mk3': () => new MPKMiniMk3Converter(),
    };

    const converterFactory = converterMap[controllerKey];
    if (!converterFactory) {
      throw new Error(
        `No converter available for ${adapter.manufacturer} ${adapter.model}. ` +
          `Supported controllers: ${Object.keys(converterMap).join(', ')}`,
      );
    }

    return converterFactory();
  }

  /**
   * Create DAW deployers for requested targets.
   * Currently a placeholder - deployers should be registered or injected.
   */
  private static async createDeployers(targets: string[]): Promise<Map<string, DAWDeployerInterface>> {
    const deployers = new Map<string, DAWDeployerInterface>();

    // This is a placeholder - in practice, deployers would be:
    // 1. Registered in a registry
    // 2. Dependency-injected via constructor
    // 3. Dynamically loaded based on available DAW implementations

    // Future implementation:
    // for (const target of targets) {
    //   const deployer = await DAWDeployerRegistry.get(target);
    //   deployers.set(target.toLowerCase(), deployer);
    // }

    // For now, throw an error to indicate missing implementation
    if (targets.length > 0) {
      throw new Error(
        `DAW deployers not yet implemented. Requested targets: ${targets.join(', ')}. ` +
          'Please provide deployers via CreateOptions.deployers.',
      );
    }

    return deployers;
  }

  /**
   * Read configuration from controller at specified slot.
   */
  private async readControllerConfiguration(slot: number): Promise<ControllerConfiguration> {
    if (!this.controllerAdapter.isConnected()) {
      await this.controllerAdapter.connect();
    }

    const config = await this.controllerAdapter.readConfiguration(slot);
    if (!config || config.controls.length === 0) {
      throw new Error(`No valid configuration found in slot ${slot}`);
    }

    return config;
  }

  /**
   * Convert controller configuration to canonical format.
   */
  private async convertToCanonical(
    config: ControllerConfiguration,
    options: WorkflowOptions,
  ): Promise<CanonicalMidiMap> {
    if (!this.converter.canConvert(config)) {
      throw new Error('Controller configuration cannot be converted to canonical format');
    }

    // Build conversion options with proper type safety for exactOptionalPropertyTypes
    const conversionOptions: ConversionOptions = {
      pluginInfo: options.pluginInfo ?? undefined,
      midiChannel: options.midiChannel ?? undefined,
      preserveLabels: options.preserveLabels ?? false,
    };

    return this.converter.convert(config, conversionOptions);
  }

  /**
   * Save canonical MIDI map to YAML file.
   */
  private async saveCanonicalYAML(map: CanonicalMidiMap, outputDir: string): Promise<string> {
    // Create output directory if it doesn't exist
    await mkdir(outputDir, { recursive: true });

    // Generate filename from map name
    const filename = this.sanitizeFilename(map.metadata.name) + '.yaml';
    const filepath = join(outputDir, filename);

    // Serialize to YAML
    const yamlContent = CanonicalMapParser.serializeToYAML(map);

    // Write to file
    await writeFile(filepath, yamlContent, 'utf-8');

    return filepath;
  }

  /**
   * Sanitize map name for use as filename.
   */
  private sanitizeFilename(name: string): string {
    return name
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-+|-+$/g, '');
  }

  /**
   * Emit progress event.
   */
  private emitProgress(step: number, message: string, data?: unknown): void {
    this.emit('progress', {
      step,
      message,
      data,
    } as ProgressEvent);
  }

  /**
   * Disconnect from controller when done.
   */
  async cleanup(): Promise<void> {
    if (this.controllerAdapter.isConnected()) {
      await this.controllerAdapter.disconnect();
    }
  }
}

/**
 * Factory function for creating DeploymentWorkflow instances.
 * Provides backward compatibility and easier instantiation.
 *
 * @param options - Creation options
 * @returns Configured DeploymentWorkflow instance
 */
export async function createDeploymentWorkflow(options: CreateOptions): Promise<DeploymentWorkflow> {
  return DeploymentWorkflow.create(options);
}
