#!/usr/bin/env node

/**
 * Maps Health Check Tool
 *
 * Performs comprehensive health checks on canonical MIDI mappings,
 * cross-validating with plugin descriptors and providing health scores.
 */

import { readFile } from 'fs/promises';
import { join, basename, extname } from 'path';
import { glob } from 'glob';

import { CanonicalMapParser } from '@/modules/canonical-midi-maps/src/parsers/yaml-parser';
import type { CanonicalMidiMapOutput } from '@/modules/canonical-midi-maps/src/validators/schema';
import type { PluginDescriptor } from '@/modules/canonical-midi-maps/src/types/plugin-descriptor';
import type { ValidationError, ValidationWarning } from '@/tools/types/workflow';

interface HealthCheckResult {
  /** Map file path */
  filePath: string;

  /** Map identifier */
  mapId: string;

  /** Overall health score (0-1) */
  healthScore: number;

  /** Health status */
  status: 'excellent' | 'good' | 'fair' | 'poor' | 'critical';

  /** Individual check results */
  checks: {
    syntax: HealthCheck;
    midi: HealthCheck;
    pluginMapping: HealthCheck;
    completeness: HealthCheck;
    consistency: HealthCheck;
    documentation: HealthCheck;
  };

  /** Summary statistics */
  stats: {
    totalControls: number;
    mappedControls: number;
    mappingCoverage: number;
    uniqueCCs: number;
    duplicateCCs: number;
    invalidCCs: number;
  };

  /** Recommendations for improvement */
  recommendations: Recommendation[];

  /** Performance metrics */
  performance: {
    checkTime: number;
    fileSize: number;
  };
}

interface HealthCheck {
  /** Check name */
  name: string;

  /** Check passed */
  passed: boolean;

  /** Check score (0-1) */
  score: number;

  /** Issues found */
  issues: HealthIssue[];

  /** Check metadata */
  metadata?: Record<string, any>;
}

interface HealthIssue {
  /** Issue severity */
  severity: 'info' | 'warning' | 'error' | 'critical';

  /** Issue message */
  message: string;

  /** Issue location */
  path?: string;

  /** Fix suggestion */
  fix?: string;
}

interface Recommendation {
  /** Recommendation priority */
  priority: 'high' | 'medium' | 'low';

  /** Recommendation category */
  category: 'mapping' | 'documentation' | 'midi' | 'plugin' | 'performance';

  /** Recommendation title */
  title: string;

  /** Detailed description */
  description: string;

  /** Potential impact */
  impact: string;

  /** Implementation effort */
  effort: 'low' | 'medium' | 'high';
}

interface CheckOptions {
  /** Plugin descriptors directory */
  pluginDescriptorsDir?: string;

  /** Include plugin cross-validation */
  checkPlugins?: boolean;

  /** Minimum health score threshold */
  minHealthScore?: number;

  /** Include detailed recommendations */
  includeRecommendations?: boolean;

  /** Output format */
  format?: 'table' | 'json' | 'detailed';

  /** Only show failed checks */
  failedOnly?: boolean;
}

class MapsHealthChecker {
  private readonly pluginDescriptors = new Map<string, PluginDescriptor>();

  constructor(private readonly options: CheckOptions = {}) {
    // Set defaults
    this.options.checkPlugins ??= true;
    this.options.minHealthScore ??= 0.8;
    this.options.includeRecommendations ??= true;
    this.options.format ??= 'table';
    this.options.failedOnly ??= false;
  }

  /**
   * Load plugin descriptors for cross-validation
   */
  async loadPluginDescriptors(descriptorsDir: string): Promise<void> {
    try {
      const descriptorFiles = await glob(join(descriptorsDir, '**/*.json'));

      for (const file of descriptorFiles) {
        try {
          const content = await readFile(file, 'utf8');
          const descriptor: PluginDescriptor = JSON.parse(content);

          const pluginId = this.generatePluginId(descriptor);
          this.pluginDescriptors.set(pluginId, descriptor);
        } catch (error) {
          console.warn(`Failed to load plugin descriptor ${file}: ${error instanceof Error ? error.message : 'Unknown error'}`);
        }
      }

      console.log(`Loaded ${this.pluginDescriptors.size} plugin descriptors for health checking`);
    } catch (error) {
      console.error(`Failed to load plugin descriptors: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  }

  /**
   * Check health of maps in directory
   */
  async checkMapsHealth(searchPath: string): Promise<HealthCheckResult[]> {
    const mapFiles = await glob(join(searchPath, '**/*.{yaml,yml,json}'));

    if (mapFiles.length === 0) {
      throw new Error(`No map files found in ${searchPath}`);
    }

    // Load plugin descriptors if needed
    if (this.options.checkPlugins && this.options.pluginDescriptorsDir) {
      await this.loadPluginDescriptors(this.options.pluginDescriptorsDir);
    }

    // Check each map
    const results: HealthCheckResult[] = [];

    for (const filePath of mapFiles) {
      try {
        const result = await this.checkMapHealth(filePath);

        // Filter by health score if threshold is set
        if (!this.options.minHealthScore || result.healthScore >= this.options.minHealthScore ||
            result.status === 'critical' || result.status === 'poor') {
          results.push(result);
        }
      } catch (error) {
        // Create failed result
        results.push({
          filePath,
          mapId: basename(filePath, extname(filePath)),
          healthScore: 0,
          status: 'critical',
          checks: this.createFailedChecks(error instanceof Error ? error.message : 'Unknown error'),
          stats: { totalControls: 0, mappedControls: 0, mappingCoverage: 0, uniqueCCs: 0, duplicateCCs: 0, invalidCCs: 0 },
          recommendations: [],
          performance: { checkTime: 0, fileSize: 0 },
        });
      }
    }

    // Sort by health score (worst first)
    results.sort((a, b) => a.healthScore - b.healthScore);

    return results;
  }

  /**
   * Check health of a single map
   */
  private async checkMapHealth(filePath: string): Promise<HealthCheckResult> {
    const startTime = performance.now();

    // Read and parse map
    const content = await readFile(filePath, 'utf8');
    const stats = await import('fs/promises').then(fs => fs.stat(filePath));
    const fileExtension = extname(filePath).toLowerCase();

    let parseResult: { map?: CanonicalMidiMapOutput; validation: any };

    if (fileExtension === '.yaml' || fileExtension === '.yml') {
      parseResult = CanonicalMapParser.parseFromYAML(content);
    } else if (fileExtension === '.json') {
      parseResult = CanonicalMapParser.parseFromJSON(content);
    } else {
      throw new Error(`Unsupported file format: ${fileExtension}`);
    }

    if (!parseResult.validation.valid || !parseResult.map) {
      throw new Error(`Invalid map format: ${parseResult.validation.errors?.[0]?.message || 'Parse error'}`);
    }

    const map = parseResult.map;

    // Perform individual health checks
    const checks = {
      syntax: this.checkSyntax(parseResult.validation),
      midi: this.checkMidiCompliance(map),
      pluginMapping: this.checkPluginMapping(map),
      completeness: this.checkCompleteness(map),
      consistency: this.checkConsistency(map),
      documentation: this.checkDocumentation(map),
    };

    // Calculate statistics
    const mapStats = this.calculateStats(map);

    // Calculate overall health score
    const healthScore = this.calculateHealthScore(checks);
    const status = this.determineHealthStatus(healthScore);

    // Generate recommendations
    const recommendations = this.options.includeRecommendations ?
      this.generateRecommendations(map, checks, mapStats) : [];

    const checkTime = performance.now() - startTime;

    return {
      filePath,
      mapId: this.generateMapId(map),
      healthScore,
      status,
      checks,
      stats: mapStats,
      recommendations,
      performance: {
        checkTime,
        fileSize: stats.size,
      },
    };
  }

  /**
   * Check syntax and schema validation
   */
  private checkSyntax(validation: any): HealthCheck {
    const issues: HealthIssue[] = [];

    // Check for validation errors
    if (validation.errors && validation.errors.length > 0) {
      validation.errors.forEach((error: ValidationError) => {
        issues.push({
          severity: 'error',
          message: error.message,
          path: error.path,
          fix: 'Fix syntax or schema validation error',
        });
      });
    }

    // Check for validation warnings
    if (validation.warnings && validation.warnings.length > 0) {
      validation.warnings.forEach((warning: ValidationWarning) => {
        issues.push({
          severity: 'warning',
          message: warning.message,
          path: warning.path,
        });
      });
    }

    const score = Math.max(0, 1 - (validation.errors?.length || 0) * 0.5 - (validation.warnings?.length || 0) * 0.1);

    return {
      name: 'Syntax & Schema',
      passed: (validation.errors?.length || 0) === 0,
      score,
      issues,
    };
  }

  /**
   * Check MIDI protocol compliance
   */
  private checkMidiCompliance(map: CanonicalMidiMapOutput): HealthCheck {
    const issues: HealthIssue[] = [];
    const ccUsage = new Map<string, string[]>();
    let invalidCCCount = 0;

    // Check MIDI channel
    if (map.midi_channel !== undefined && (map.midi_channel < 1 || map.midi_channel > 16)) {
      issues.push({
        severity: 'error',
        message: `Invalid MIDI channel: ${map.midi_channel} (must be 1-16)`,
        path: 'midi_channel',
        fix: 'Set MIDI channel to value between 1 and 16',
      });
    }

    // Check controls
    map.controls.forEach((control, index) => {
      // Check CC numbers
      if (control.cc !== undefined) {
        if (control.cc < 0 || control.cc > 127) {
          invalidCCCount++;
          issues.push({
            severity: 'error',
            message: `Invalid CC number: ${control.cc} (must be 0-127)`,
            path: `controls[${index}].cc`,
            fix: 'Use CC number between 0 and 127',
          });
        }

        // Track CC usage
        const channelKey = typeof control.channel === 'string' ? control.channel :
                          typeof control.channel === 'number' ? control.channel.toString() :
                          map.midi_channel?.toString() || 'global';
        const ccKey = `${channelKey}:${control.cc}`;

        if (!ccUsage.has(ccKey)) {
          ccUsage.set(ccKey, []);
        }
        ccUsage.get(ccKey)!.push(control.id);
      }

      // Check channel references
      if (typeof control.channel === 'number' && (control.channel < 1 || control.channel > 16)) {
        issues.push({
          severity: 'error',
          message: `Invalid control channel: ${control.channel} (must be 1-16)`,
          path: `controls[${index}].channel`,
          fix: 'Set channel to value between 1 and 16',
        });
      }

      // Check button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach((button, buttonIndex) => {
          if (button.cc < 0 || button.cc > 127) {
            invalidCCCount++;
            issues.push({
              severity: 'error',
              message: `Invalid button CC: ${button.cc} (must be 0-127)`,
              path: `controls[${index}].buttons[${buttonIndex}].cc`,
              fix: 'Use CC number between 0 and 127',
            });
          }
        });
      }
    });

    // Check for duplicate CC assignments
    const duplicateCount = Array.from(ccUsage.values()).filter(controls => controls.length > 1).length;
    ccUsage.forEach((controlIds, ccKey) => {
      if (controlIds.length > 1) {
        issues.push({
          severity: 'warning',
          message: `Duplicate CC assignment ${ccKey} used by: ${controlIds.join(', ')}`,
          fix: 'Use unique CC numbers for each control',
        });
      }
    });

    const score = Math.max(0, 1 - invalidCCCount * 0.3 - duplicateCount * 0.1);

    return {
      name: 'MIDI Compliance',
      passed: invalidCCCount === 0,
      score,
      issues,
      metadata: {
        duplicateCCs: duplicateCount,
        invalidCCs: invalidCCCount,
        totalCCs: Array.from(ccUsage.keys()).length,
      },
    };
  }

  /**
   * Check plugin parameter mappings
   */
  private checkPluginMapping(map: CanonicalMidiMapOutput): HealthCheck {
    const issues: HealthIssue[] = [];
    let score = 1.0;

    if (!map.plugin) {
      return {
        name: 'Plugin Mapping',
        passed: true,
        score: 1.0,
        issues: [{
          severity: 'info',
          message: 'No plugin specified - generic control mapping',
        }],
      };
    }

    // Find plugin descriptor
    const pluginId = this.generatePluginId({
      plugin: {
        manufacturer: map.plugin.manufacturer,
        name: map.plugin.name,
        version: map.plugin.version || '',
        format: map.plugin.format || 'VST3',
      },
    });

    const descriptor = this.pluginDescriptors.get(pluginId);
    if (!descriptor && this.options.checkPlugins) {
      issues.push({
        severity: 'warning',
        message: `No plugin descriptor found for ${map.plugin.manufacturer} ${map.plugin.name}`,
        fix: 'Add plugin descriptor or disable plugin validation',
      });
      score = 0.7;
    } else if (descriptor) {
      // Validate mappings against descriptor
      const availableParams = new Map(descriptor.parameters.map(p => [p.index.toString(), p]));
      let invalidMappings = 0;

      map.controls.forEach((control, index) => {
        if (control.plugin_parameter !== undefined) {
          const param = availableParams.get(control.plugin_parameter.toString());
          if (!param) {
            invalidMappings++;
            issues.push({
              severity: 'error',
              message: `Invalid plugin parameter ${control.plugin_parameter} for control ${control.id}`,
              path: `controls[${index}].plugin_parameter`,
              fix: `Use valid parameter index (0-${descriptor.parameters.length - 1})`,
            });
          } else if (param.automatable === false) {
            issues.push({
              severity: 'warning',
              message: `Parameter ${param.name} is not automatable`,
              path: `controls[${index}].plugin_parameter`,
            });
          }
        }
      });

      score = Math.max(0, 1 - invalidMappings * 0.2);
    }

    return {
      name: 'Plugin Mapping',
      passed: issues.filter(i => i.severity === 'error').length === 0,
      score,
      issues,
    };
  }

  /**
   * Check mapping completeness
   */
  private checkCompleteness(map: CanonicalMidiMapOutput): HealthCheck {
    const issues: HealthIssue[] = [];

    const totalControls = map.controls.length;
    let mappedControls = 0;
    let unmappedControls: string[] = [];

    map.controls.forEach(control => {
      if (control.plugin_parameter !== undefined) {
        mappedControls++;
      } else {
        unmappedControls.push(control.id);
      }

      // Check button groups
      if (control.type === 'button_group' && control.buttons) {
        control.buttons.forEach(button => {
          if (button.plugin_parameter !== undefined) {
            mappedControls++;
          } else {
            unmappedControls.push(`${control.id}.${button.id}`);
          }
        });
      }
    });

    const mappingCoverage = totalControls > 0 ? mappedControls / totalControls : 0;

    if (mappingCoverage < 0.5) {
      issues.push({
        severity: 'warning',
        message: `Low mapping coverage: ${(mappingCoverage * 100).toFixed(1)}% of controls mapped`,
        fix: 'Add plugin parameter mappings for more controls',
      });
    }

    if (unmappedControls.length > 0) {
      issues.push({
        severity: 'info',
        message: `${unmappedControls.length} unmapped controls: ${unmappedControls.slice(0, 5).join(', ')}${unmappedControls.length > 5 ? '...' : ''}`,
      });
    }

    return {
      name: 'Mapping Completeness',
      passed: mappingCoverage >= 0.7,
      score: mappingCoverage,
      issues,
      metadata: {
        totalControls,
        mappedControls,
        mappingCoverage,
      },
    };
  }

  /**
   * Check internal consistency
   */
  private checkConsistency(map: CanonicalMidiMapOutput): HealthCheck {
    const issues: HealthIssue[] = [];

    // Check naming consistency
    const controlNames = map.controls.map(c => c.name);
    const duplicateNames = controlNames.filter((name, index, arr) => arr.indexOf(name) !== index);

    if (duplicateNames.length > 0) {
      issues.push({
        severity: 'warning',
        message: `Duplicate control names: ${[...new Set(duplicateNames)].join(', ')}`,
        fix: 'Use unique names for all controls',
      });
    }

    // Check ID consistency
    const controlIds = map.controls.map(c => c.id);
    const duplicateIds = controlIds.filter((id, index, arr) => arr.indexOf(id) !== index);

    if (duplicateIds.length > 0) {
      issues.push({
        severity: 'error',
        message: `Duplicate control IDs: ${[...new Set(duplicateIds)].join(', ')}`,
        fix: 'Use unique IDs for all controls',
      });
    }

    // Check range consistency
    let invalidRanges = 0;
    map.controls.forEach((control, index) => {
      if (control.range) {
        const [min, max] = control.range;
        if (min >= max) {
          invalidRanges++;
          issues.push({
            severity: 'error',
            message: `Invalid range [${min}, ${max}] for control ${control.id}`,
            path: `controls[${index}].range`,
            fix: 'Ensure minimum value is less than maximum value',
          });
        }
      }
    });

    const score = Math.max(0, 1 - duplicateIds.length * 0.3 - duplicateNames.length * 0.1 - invalidRanges * 0.2);

    return {
      name: 'Consistency',
      passed: duplicateIds.length === 0 && invalidRanges === 0,
      score,
      issues,
    };
  }

  /**
   * Check documentation quality
   */
  private checkDocumentation(map: CanonicalMidiMapOutput): HealthCheck {
    const issues: HealthIssue[] = [];
    let score = 0;

    // Check required metadata
    const requiredFields = ['name', 'description', 'author', 'date'];
    requiredFields.forEach(field => {
      if (!map.metadata[field as keyof typeof map.metadata]) {
        issues.push({
          severity: 'info',
          message: `Missing ${field} in metadata`,
          path: `metadata.${field}`,
          fix: `Add ${field} to improve documentation`,
        });
      } else {
        score += 0.2;
      }
    });

    // Check control descriptions
    const undocumentedControls = map.controls.filter(c => !c.description).length;
    if (undocumentedControls > 0) {
      issues.push({
        severity: 'info',
        message: `${undocumentedControls} controls missing descriptions`,
        fix: 'Add descriptions to improve usability',
      });
    } else {
      score += 0.2;
    }

    // Check tags
    if (!map.metadata.tags || map.metadata.tags.length === 0) {
      issues.push({
        severity: 'info',
        message: 'No tags specified for categorization',
        path: 'metadata.tags',
        fix: 'Add relevant tags for better organization',
      });
    }

    return {
      name: 'Documentation',
      passed: score >= 0.6,
      score: Math.min(1, score),
      issues,
    };
  }

  /**
   * Calculate overall health score
   */
  private calculateHealthScore(checks: HealthCheckResult['checks']): number {
    const weights = {
      syntax: 0.25,
      midi: 0.20,
      pluginMapping: 0.20,
      completeness: 0.15,
      consistency: 0.15,
      documentation: 0.05,
    };

    return Object.entries(checks).reduce((score, [checkName, check]) => {
      const weight = weights[checkName as keyof typeof weights] || 0;
      return score + (check.score * weight);
    }, 0);
  }

  /**
   * Determine health status from score
   */
  private determineHealthStatus(score: number): HealthCheckResult['status'] {
    if (score >= 0.9) return 'excellent';
    if (score >= 0.8) return 'good';
    if (score >= 0.6) return 'fair';
    if (score >= 0.4) return 'poor';
    return 'critical';
  }

  /**
   * Calculate map statistics
   */
  private calculateStats(map: CanonicalMidiMapOutput): HealthCheckResult['stats'] {
    let totalControls = map.controls.length;
    let mappedControls = 0;
    const ccNumbers = new Set<string>();
    const duplicateCCs = new Set<string>();
    let invalidCCs = 0;

    map.controls.forEach(control => {
      if (control.plugin_parameter !== undefined) {
        mappedControls++;
      }

      if (control.cc !== undefined) {
        if (control.cc < 0 || control.cc > 127) {
          invalidCCs++;
        } else {
          const ccKey = `${control.channel || 'global'}:${control.cc}`;
          if (ccNumbers.has(ccKey)) {
            duplicateCCs.add(ccKey);
          } else {
            ccNumbers.add(ccKey);
          }
        }
      }

      // Check button groups
      if (control.type === 'button_group' && control.buttons) {
        totalControls += control.buttons.length;
        control.buttons.forEach(button => {
          if (button.plugin_parameter !== undefined) {
            mappedControls++;
          }
        });
      }
    });

    return {
      totalControls,
      mappedControls,
      mappingCoverage: totalControls > 0 ? mappedControls / totalControls : 0,
      uniqueCCs: ccNumbers.size,
      duplicateCCs: duplicateCCs.size,
      invalidCCs,
    };
  }

  /**
   * Generate improvement recommendations
   */
  private generateRecommendations(
    map: CanonicalMidiMapOutput,
    checks: HealthCheckResult['checks'],
    stats: HealthCheckResult['stats']
  ): Recommendation[] {
    const recommendations: Recommendation[] = [];

    // Plugin mapping recommendations
    if (stats.mappingCoverage < 0.7 && map.plugin) {
      recommendations.push({
        priority: 'high',
        category: 'mapping',
        title: 'Improve Plugin Mapping Coverage',
        description: `Only ${(stats.mappingCoverage * 100).toFixed(1)}% of controls are mapped to plugin parameters`,
        impact: 'Increases plugin control functionality and user experience',
        effort: 'medium',
      });
    }

    // MIDI optimization recommendations
    if (stats.duplicateCCs > 0) {
      recommendations.push({
        priority: 'medium',
        category: 'midi',
        title: 'Resolve Duplicate CC Assignments',
        description: `${stats.duplicateCCs} CC numbers are used by multiple controls`,
        impact: 'Prevents MIDI conflicts and unpredictable behavior',
        effort: 'low',
      });
    }

    // Documentation recommendations
    if (checks.documentation.score < 0.6) {
      recommendations.push({
        priority: 'low',
        category: 'documentation',
        title: 'Enhance Documentation',
        description: 'Add missing metadata fields and control descriptions',
        impact: 'Improves maintainability and user understanding',
        effort: 'low',
      });
    }

    return recommendations;
  }

  /**
   * Output health check results
   */
  outputResults(results: HealthCheckResult[]): void {
    if (results.length === 0) {
      console.log('No maps found or all maps passed health checks.');
      return;
    }

    switch (this.options.format) {
      case 'json':
        console.log(JSON.stringify(results, null, 2));
        break;
      case 'detailed':
        this.outputDetailed(results);
        break;
      case 'table':
      default:
        this.outputTable(results);
        break;
    }
  }

  /**
   * Output results as table
   */
  private outputTable(results: HealthCheckResult[]): void {
    console.log(`Health Check Results (${results.length} maps):\n`);

    // Summary table
    console.log('Map Name'.padEnd(30) + 'Status'.padEnd(12) + 'Score'.padEnd(8) + 'Issues');
    console.log('-'.repeat(65));

    results.forEach(result => {
      const name = basename(result.filePath, extname(result.filePath)).padEnd(30);
      const status = this.getStatusEmoji(result.status).padEnd(12);
      const score = `${(result.healthScore * 100).toFixed(1)}%`.padEnd(8);
      const issueCount = Object.values(result.checks)
        .reduce((sum, check) => sum + check.issues.filter(i => i.severity === 'error').length, 0);
      const issues = issueCount > 0 ? `${issueCount} errors` : 'None';

      console.log(`${name}${status}${score}${issues}`);
    });

    // Overall statistics
    const avgScore = results.reduce((sum, r) => sum + r.healthScore, 0) / results.length;
    const statusCounts = results.reduce((counts, r) => {
      counts[r.status] = (counts[r.status] || 0) + 1;
      return counts;
    }, {} as Record<string, number>);

    console.log(`\nOverall Health: ${(avgScore * 100).toFixed(1)}%`);
    console.log('Status Distribution:', Object.entries(statusCounts)
      .map(([status, count]) => `${status}: ${count}`)
      .join(', '));
  }

  /**
   * Output detailed results
   */
  private outputDetailed(results: HealthCheckResult[]): void {
    results.forEach((result, index) => {
      if (index > 0) console.log('\n' + '='.repeat(80));

      console.log(`\n${this.getStatusEmoji(result.status)} ${basename(result.filePath)} (${(result.healthScore * 100).toFixed(1)}%)`);
      console.log(`File: ${result.filePath}`);
      console.log(`Status: ${result.status.toUpperCase()}`);

      // Check results
      console.log('\nHealth Checks:');
      Object.values(result.checks).forEach(check => {
        const icon = check.passed ? '✅' : '❌';
        const score = `${(check.score * 100).toFixed(1)}%`;
        console.log(`  ${icon} ${check.name}: ${score}`);

        if (check.issues.length > 0 && (!this.options.failedOnly || !check.passed)) {
          check.issues.forEach(issue => {
            const severity = this.getSeverityEmoji(issue.severity);
            console.log(`    ${severity} ${issue.message}`);
            if (issue.fix) {
              console.log(`       Fix: ${issue.fix}`);
            }
          });
        }
      });

      // Statistics
      console.log('\nStatistics:');
      console.log(`  Controls: ${result.stats.totalControls} (${result.stats.mappedControls} mapped, ${(result.stats.mappingCoverage * 100).toFixed(1)}%)`);
      console.log(`  MIDI CCs: ${result.stats.uniqueCCs} unique, ${result.stats.duplicateCCs} duplicates, ${result.stats.invalidCCs} invalid`);

      // Recommendations
      if (result.recommendations.length > 0) {
        console.log('\nRecommendations:');
        result.recommendations.forEach(rec => {
          const priority = this.getPriorityEmoji(rec.priority);
          console.log(`  ${priority} ${rec.title} (${rec.effort} effort)`);
          console.log(`     ${rec.description}`);
          console.log(`     Impact: ${rec.impact}`);
        });
      }
    });
  }

  private getStatusEmoji(status: HealthCheckResult['status']): string {
    switch (status) {
      case 'excellent': return '🟢 Excellent';
      case 'good': return '🟡 Good     ';
      case 'fair': return '🟠 Fair     ';
      case 'poor': return '🔴 Poor     ';
      case 'critical': return '💀 Critical ';
    }
  }

  private getSeverityEmoji(severity: HealthIssue['severity']): string {
    switch (severity) {
      case 'info': return 'ℹ️';
      case 'warning': return '⚠️';
      case 'error': return '❌';
      case 'critical': return '💀';
    }
  }

  private getPriorityEmoji(priority: Recommendation['priority']): string {
    switch (priority) {
      case 'high': return '🔴';
      case 'medium': return '🟡';
      case 'low': return '🟢';
    }
  }

  private createFailedChecks(errorMessage: string): HealthCheckResult['checks'] {
    const failedCheck: HealthCheck = {
      name: 'Parse Error',
      passed: false,
      score: 0,
      issues: [{
        severity: 'critical',
        message: errorMessage,
      }],
    };

    return {
      syntax: failedCheck,
      midi: failedCheck,
      pluginMapping: failedCheck,
      completeness: failedCheck,
      consistency: failedCheck,
      documentation: failedCheck,
    };
  }

  private generatePluginId(descriptor: { plugin: { manufacturer: string; name: string } }): string {
    return `${descriptor.plugin.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${descriptor.plugin.name.toLowerCase().replace(/\s+/g, '-')}`;
  }

  private generateMapId(map: CanonicalMidiMapOutput): string {
    const device = `${map.device.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.device.model.toLowerCase().replace(/\s+/g, '-')}`;
    const plugin = map.plugin ? `_${map.plugin.manufacturer.toLowerCase().replace(/\s+/g, '-')}_${map.plugin.name.toLowerCase().replace(/\s+/g, '-')}` : '';
    return device + plugin;
  }
}

// CLI interface
async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0) {
    console.error('Usage: check.ts <maps-directory> [options]');
    console.error('');
    console.error('Options:');
    console.error('  --plugin-descriptors <dir>  Plugin descriptors directory');
    console.error('  --no-plugin-check           Skip plugin validation');
    console.error('  --min-score <score>         Minimum health score threshold (0-1)');
    console.error('  --no-recommendations        Skip recommendations generation');
    console.error('  --format <format>           Output format: table, json, detailed');
    console.error('  --failed-only               Show only failed checks in detailed mode');
    process.exit(1);
  }

  const searchPath = args[0];
  const options: CheckOptions = {
    checkPlugins: true,
    includeRecommendations: true,
    format: 'table',
    failedOnly: false,
  };

  for (let i = 1; i < args.length; i++) {
    switch (args[i]) {
      case '--plugin-descriptors':
        options.pluginDescriptorsDir = args[++i];
        break;
      case '--no-plugin-check':
        options.checkPlugins = false;
        break;
      case '--min-score':
        options.minHealthScore = parseFloat(args[++i]);
        break;
      case '--no-recommendations':
        options.includeRecommendations = false;
        break;
      case '--format':
        options.format = args[++i] as 'table' | 'json' | 'detailed';
        break;
      case '--failed-only':
        options.failedOnly = true;
        break;
    }
  }

  try {
    const checker = new MapsHealthChecker(options);
    const results = await checker.checkMapsHealth(searchPath);
    checker.outputResults(results);

    // Exit with error code if any critical issues found
    const hasCriticalIssues = results.some(r => r.status === 'critical');
    process.exit(hasCriticalIssues ? 1 : 0);
  } catch (error) {
    console.error(`Error: ${error instanceof Error ? error.message : 'Unknown error'}`);
    process.exit(1);
  }
}

if (require.main === module) {
  main().catch(console.error);
}

export { MapsHealthChecker, type HealthCheckResult, type CheckOptions };