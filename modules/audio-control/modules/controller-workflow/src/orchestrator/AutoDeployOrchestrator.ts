/**
 * AutoDeployOrchestrator - Coordinates automatic deployment of all controller slots
 *
 * This orchestrator:
 * 1. Reads all 16 slots from the connected controller
 * 2. Identifies which plugin each slot is designed for (AI)
 * 3. Matches control names to plugin parameters (AI)
 * 4. Deploys to all specified DAWs
 * 5. Aggregates results and provides summary
 *
 * @module AutoDeployOrchestrator
 */

import type { ControllerAdapterInterface } from '../types/controller-adapter.js';
import type { CanonicalConverterInterface } from '../types/canonical-converter.js';
import type { DAWDeployerInterface } from '../types/daw-deployer.js';
import type { PluginIdentifierInterface, PluginIdentificationResult } from '../services/PluginIdentifier.js';
import type { ParameterMatcherInterface, ParameterMatchResult } from '../services/ParameterMatcher.js';
import type { CanonicalMidiMap } from '@oletizi/canonical-midi-maps';

/**
 * Options for auto-deployment
 */
export interface AutoDeployOptions {
  /** DAW deployers to use */
  deployers: DAWDeployerInterface[];

  /** Output directory for generated files */
  outputDir: string;

  /** Whether to auto-install to DAW directories */
  autoInstall?: boolean;

  /** MIDI channel override (1-16) */
  midiChannel?: number;

  /** Dry-run mode (no files written) */
  dryRun?: boolean;

  /** Minimum confidence for plugin identification (default: 0.7) */
  minConfidence?: number;

  /** Whether to skip slots with low confidence matches */
  skipLowConfidence?: boolean;
}

/**
 * Result for a single slot deployment
 */
export interface SlotDeploymentResult {
  /** Slot index */
  slotIndex: number;

  /** Slot name/description */
  slotName: string;

  /** Number of controls in this slot */
  controlCount: number;

  /** Plugin identification result */
  pluginIdentification: PluginIdentificationResult;

  /** Parameter matching result (if plugin identified) */
  parameterMatching?: {
    totalControls: number;
    matchedControls: number;
    highConfidenceMatches: number;
    lowConfidenceMatches: number;
  };

  /** Deployment results by DAW */
  deploymentResults: {
    daw: string;
    success: boolean;
    outputFiles?: string[];
    error?: string;
  }[];

  /** Whether deployment was successful */
  success: boolean;

  /** Overall error message (if failed) */
  error?: string;

  /** Time taken for this slot (milliseconds) */
  duration: number;
}

/**
 * Overall auto-deployment result
 */
export interface AutoDeploymentResult {
  /** Total number of slots checked */
  totalSlots: number;

  /** Number of populated slots found */
  populatedSlots: number;

  /** Number of slots successfully processed */
  slotsProcessed: number;

  /** Number of slots successfully deployed */
  slotsDeployed: number;

  /** Results for each slot */
  slotResults: SlotDeploymentResult[];

  /** Total time taken (milliseconds) */
  totalTime: number;

  /** Overall success */
  success: boolean;
}

/**
 * AutoDeployOrchestrator implementation
 */
export class AutoDeployOrchestrator {
  private adapter: ControllerAdapterInterface;
  private converter: CanonicalConverterInterface;
  private pluginIdentifier: PluginIdentifierInterface;
  private parameterMatcher: ParameterMatcherInterface;

  constructor(
    adapter: ControllerAdapterInterface,
    converter: CanonicalConverterInterface,
    pluginIdentifier: PluginIdentifierInterface,
    parameterMatcher: ParameterMatcherInterface
  ) {
    this.adapter = adapter;
    this.converter = converter;
    this.pluginIdentifier = pluginIdentifier;
    this.parameterMatcher = parameterMatcher;
  }

  /**
   * Execute auto-deployment for all controller slots
   */
  async deploy(options: AutoDeployOptions): Promise<AutoDeploymentResult> {
    const startTime = Date.now();
    const slotResults: SlotDeploymentResult[] = [];

    const minConfidence = options.minConfidence ?? 0.7;

    try {
      // Step 1: Connect to controller
      await this.adapter.connect();

      // Step 2: List all configurations (16 slots)
      const slots = await this.adapter.listConfigurations();

      let populatedSlots = 0;
      let slotsProcessed = 0;
      let slotsDeployed = 0;

      // Step 3: Process each slot
      for (const slot of slots) {
        if (slot.isEmpty) {
          continue; // Skip empty slots
        }

        populatedSlots++;

        const slotStartTime = Date.now();

        try {
          // Step 3a: Read configuration
          const config = await this.adapter.readConfiguration(slot.index);

          if (!config) {
            slotResults.push({
              slotIndex: slot.index,
              slotName: slot.name || `Slot ${slot.index}`,
              controlCount: 0,
              pluginIdentification: {
                pluginName: null,
                confidence: 0,
                reasoning: 'Failed to read configuration',
                claudeAvailable: false
              },
              deploymentResults: [],
              success: false,
              error: 'Failed to read configuration',
              duration: Date.now() - slotStartTime
            });
            continue;
          }

          // Step 3b: Extract control names
          const controlNames = config.controls
            .map((c: any) => c.name)
            .filter((name: any): name is string => name && name.length > 0);

          // Step 3c: Identify plugin using AI
          const pluginIdentification = await this.pluginIdentifier.identifyPlugin(
            controlNames,
            config.name
          );

          // Check if we have a confident match
          if (!pluginIdentification.pluginName || pluginIdentification.confidence < minConfidence) {
            if (options.skipLowConfidence) {
              slotResults.push({
                slotIndex: slot.index,
                slotName: config.name,
                controlCount: config.controls.length,
                pluginIdentification,
                deploymentResults: [],
                success: false,
                error: `Skipped: Low confidence match (${pluginIdentification.confidence.toFixed(2)})`,
                duration: Date.now() - slotStartTime
              });
              continue;
            }
          }

          // Step 3d: Convert to canonical format
          let canonicalMap = await this.converter.convert(config, {});

          // Step 3e: Match parameters if plugin identified
          let parameterMatching: SlotDeploymentResult['parameterMatching'];

          if (pluginIdentification.pluginName && pluginIdentification.confidence >= minConfidence) {
            try {
              // Load plugin descriptor
              const { loadPluginDescriptor } = await import('../services/ParameterMatcher.js');
              const pluginDescriptor = await loadPluginDescriptor(pluginIdentification.pluginName);

              // Match parameters
              const matches = await this.parameterMatcher.matchParameters(
                controlNames,
                pluginDescriptor
              );

              if (matches.length > 0) {
                // Update canonical map with plugin parameters
                canonicalMap = this.applyParameterMatches(canonicalMap, matches);

                const matchedControls = matches.filter((m: any) => m.pluginParameter !== undefined);
                parameterMatching = {
                  totalControls: controlNames.length,
                  matchedControls: matchedControls.length,
                  highConfidenceMatches: matchedControls.filter((m: any) => m.confidence >= 0.7).length,
                  lowConfidenceMatches: matchedControls.filter((m: any) => m.confidence < 0.7).length
                };
              }
            } catch (error: any) {
              // Parameter matching failed, but continue with canonical deployment
              console.warn(`Parameter matching failed for slot ${slot.index}: ${error.message}`);
            }
          }

          slotsProcessed++;

          // Step 3f: Deploy to all DAWs
          const deploymentResults = await this.deployToDAWs(
            canonicalMap,
            options.deployers,
            options.outputDir,
            options.dryRun || false
          );

          const allDeployed = deploymentResults.every(r => r.success);
          if (allDeployed) {
            slotsDeployed++;
          }

          const slotResult: SlotDeploymentResult = {
            slotIndex: slot.index,
            slotName: config.name,
            controlCount: config.controls.length,
            pluginIdentification,
            deploymentResults,
            success: allDeployed,
            duration: Date.now() - slotStartTime
          };

          if (parameterMatching) {
            slotResult.parameterMatching = parameterMatching;
          }

          slotResults.push(slotResult);
        } catch (error: any) {
          slotResults.push({
            slotIndex: slot.index,
            slotName: slot.name || `Slot ${slot.index}`,
            controlCount: 0,
            pluginIdentification: {
              pluginName: null,
              confidence: 0,
              reasoning: `Error: ${error.message}`,
              claudeAvailable: false
            },
            deploymentResults: [],
            success: false,
            error: error.message,
            duration: Date.now() - slotStartTime
          });
        }
      }

      const totalTime = Date.now() - startTime;

      return {
        totalSlots: slots.length,
        populatedSlots,
        slotsProcessed,
        slotsDeployed,
        slotResults,
        totalTime,
        success: slotsDeployed > 0
      };
    } catch (error: any) {
      throw new Error(`Auto-deployment failed: ${error.message}`);
    } finally {
      await this.adapter.disconnect();
    }
  }

  /**
   * Apply parameter matches to canonical map
   */
  private applyParameterMatches(
    canonicalMap: CanonicalMidiMap,
    matches: ParameterMatchResult[]
  ): CanonicalMidiMap {
    const updatedControls = canonicalMap.controls.map((control: any) => {
      const match = matches.find((m: any) => m.controlName === control.name);
      if (match && match.pluginParameter !== undefined) {
        return {
          ...control,
          plugin_parameter: match.pluginParameter
        };
      }
      return control;
    });

    return {
      ...canonicalMap,
      controls: updatedControls
    };
  }

  /**
   * Deploy canonical map to all DAWs
   */
  private async deployToDAWs(
    canonicalMap: CanonicalMidiMap,
    deployers: DAWDeployerInterface[],
    outputDir: string,
    dryRun: boolean
  ): Promise<SlotDeploymentResult['deploymentResults']> {
    const results: SlotDeploymentResult['deploymentResults'] = [];

    for (const deployer of deployers) {
      try {
        const result = await deployer.deploy(canonicalMap, {
          outputPath: outputDir,
          dryRun
        });

        const deployResult: SlotDeploymentResult['deploymentResults'][0] = {
          daw: deployer.dawName,
          success: result.success
        };

        if (result.outputPath) {
          deployResult.outputFiles = [result.outputPath];
        }
        if (result.errors && result.errors.length > 0) {
          deployResult.error = result.errors.join(', ');
        }

        results.push(deployResult);
      } catch (error: any) {
        results.push({
          daw: deployer.dawName,
          success: false,
          error: error.message
        });
      }
    }

    return results;
  }

  /**
   * Factory method to create orchestrator
   */
  static create(
    adapter: ControllerAdapterInterface,
    converter: CanonicalConverterInterface,
    pluginIdentifier: PluginIdentifierInterface,
    parameterMatcher: ParameterMatcherInterface
  ): AutoDeployOrchestrator {
    return new AutoDeployOrchestrator(adapter, converter, pluginIdentifier, parameterMatcher);
  }
}
