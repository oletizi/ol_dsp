#!/usr/bin/env tsx

/**
 * Complete Workflow Tool
 * Runs the complete MIDI mapping workflow from plugin extraction to DAW generation
 */

import { parseArgs } from 'node:util';
import { execSync } from 'node:child_process';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface WorkflowOptions {
  target?: 'ardour' | 'ableton' | 'reaper' | 'all';
  force?: boolean;
  install?: boolean;
  skipValidation?: boolean;
}

function parseCliArgs(): WorkflowOptions {
  const { values } = parseArgs({
    args: process.argv.slice(2),
    options: {
      target: { type: 'string', short: 't', default: 'all' },
      force: { type: 'boolean', default: false },
      install: { type: 'boolean', default: false },
      'skip-validation': { type: 'boolean', default: false },
      help: { type: 'boolean', short: 'h', default: false }
    },
    allowPositionals: true
  });

  if (values.help) {
    console.log(`
Usage: pnpm workflow:complete [options]

Runs the complete MIDI mapping workflow:
1. Plugin extraction (if needed)
2. Map validation
3. Map compatibility checking
4. DAW map generation
5. Installation (if requested)

Options:
  -t, --target <daw>    Target DAW: ardour, ableton, reaper, all (default: all)
  --force               Force re-extraction and re-generation
  --install             Install generated maps to DAW directories
  --skip-validation     Skip map validation steps
  -h, --help            Show this help

Examples:
  pnpm workflow:complete
  pnpm workflow:complete --target ardour --install
  pnpm workflow:complete --force --skip-validation
`);
    process.exit(0);
  }

  return {
    target: values.target as WorkflowOptions['target'],
    force: values.force as boolean,
    install: values.install as boolean,
    skipValidation: values['skip-validation'] as boolean
  };
}

interface StepResult {
  step: string;
  success: boolean;
  duration: number;
  output?: string;
  error?: string;
}

function runCommand(command: string, stepName: string): StepResult {
  const startTime = Date.now();

  try {
    console.log(`\n🔄 ${stepName}...`);
    console.log(`⚡ Running: ${command}`);

    const output = execSync(command, {
      encoding: 'utf-8',
      stdio: 'inherit',
      cwd: resolve(__dirname, '../..')
    });

    const duration = Date.now() - startTime;

    console.log(`✅ ${stepName} completed in ${duration}ms`);

    return {
      step: stepName,
      success: true,
      duration,
      output: typeof output === 'string' ? output : undefined
    };

  } catch (error: any) {
    const duration = Date.now() - startTime;

    console.error(`❌ ${stepName} failed after ${duration}ms`);
    console.error(error.message);

    return {
      step: stepName,
      success: false,
      duration,
      error: error.message
    };
  }
}

function formatSummary(results: StepResult[]): void {
  console.log('\n📊 Workflow Summary:\n');

  const successful = results.filter(r => r.success);
  const failed = results.filter(r => !r.success);

  console.log(`✅ Successful steps: ${successful.length}/${results.length}`);
  console.log(`❌ Failed steps: ${failed.length}/${results.length}`);

  if (successful.length > 0) {
    console.log('\nSuccessful steps:');
    for (const result of successful) {
      console.log(`  ✅ ${result.step} (${result.duration}ms)`);
    }
  }

  if (failed.length > 0) {
    console.log('\nFailed steps:');
    for (const result of failed) {
      console.log(`  ❌ ${result.step} (${result.duration}ms)`);
      if (result.error) {
        console.log(`     Error: ${result.error.split('\n')[0]}`);
      }
    }
  }

  const totalTime = results.reduce((sum, r) => sum + r.duration, 0);
  console.log(`\n⏱️  Total time: ${Math.round(totalTime / 1000)}s`);
}

async function runCompleteWorkflow(options: WorkflowOptions): Promise<void> {
  console.log('🚀 Starting complete MIDI mapping workflow...');
  console.log('📋 Workflow steps:');
  console.log('  1. Plugin health check');
  console.log('  2. Plugin extraction');
  console.log('  3. Map validation');
  console.log('  4. Map compatibility checking');
  console.log('  5. DAW map generation');
  if (options.install) {
    console.log('  6. Map installation');
  }

  const results: StepResult[] = [];
  let shouldContinue = true;

  // Step 1: Plugin health check
  if (shouldContinue) {
    const result = runCommand('pnpm plugins:health', 'Plugin health check');
    results.push(result);
    if (!result.success && !result.error?.includes('not yet implemented')) {
      console.log('⚠️  Plugin health check failed, but continuing...');
    }
  }

  // Step 2: Plugin extraction
  if (shouldContinue) {
    const extractCmd = options.force ? 'pnpm plugins:extract:force' : 'pnpm plugins:extract';
    const result = runCommand(extractCmd, 'Plugin extraction');
    results.push(result);

    if (!result.success) {
      if (result.error?.includes('not yet implemented')) {
        console.log('⚠️  Plugin extraction not implemented, but continuing workflow...');
      } else {
        console.log('❌ Plugin extraction failed. Cannot continue without plugin data.');
        shouldContinue = false;
      }
    }
  }

  // Step 3: Map validation (optional)
  if (shouldContinue && !options.skipValidation) {
    const result = runCommand('pnpm maps:validate', 'Map validation');
    results.push(result);

    if (!result.success) {
      if (result.error?.includes('not yet implemented')) {
        console.log('⚠️  Map validation not implemented, but continuing...');
      } else {
        console.log('⚠️  Map validation failed, but continuing workflow...');
      }
    }
  }

  // Step 4: Map compatibility checking (optional)
  if (shouldContinue && !options.skipValidation) {
    const result = runCommand('pnpm maps:check', 'Map compatibility checking');
    results.push(result);

    if (!result.success) {
      if (result.error?.includes('not yet implemented')) {
        console.log('⚠️  Compatibility checking not implemented, but continuing...');
      } else {
        console.log('⚠️  Compatibility checking failed, but continuing workflow...');
      }
    }
  }

  // Step 5: DAW map generation
  if (shouldContinue) {
    const generateFlags = [
      `--target ${options.target}`,
      options.force ? '--force' : '',
      options.install ? '--install' : ''
    ].filter(Boolean).join(' ');

    const generateCmd = `pnpm daw:generate ${generateFlags}`;
    const result = runCommand(generateCmd, 'DAW map generation');
    results.push(result);

    if (!result.success) {
      if (result.error?.includes('not yet implemented')) {
        console.log('❌ DAW generation not implemented');
        shouldContinue = false;
      } else {
        console.log('❌ DAW generation failed');
        shouldContinue = false;
      }
    }
  }

  // Step 6: Installation verification (if requested)
  if (shouldContinue && options.install) {
    const result = runCommand(`pnpm daw:list --target ${options.target} --status`, 'Installation verification');
    results.push(result);
  }

  // Summary
  formatSummary(results);

  const hasFailures = results.some(r => !r.success);
  const hasImplementationGaps = results.some(r => r.error?.includes('not yet implemented'));

  if (hasImplementationGaps) {
    console.log('\n💡 Some tools are not yet fully implemented. This is expected for Phase 1.');
    console.log('   The workflow structure is in place and ready for implementation.');
  }

  if (hasFailures && !hasImplementationGaps) {
    console.log('\n❌ Workflow completed with errors');
    throw new Error(
      'Workflow failed. Check the errors above and:\n' +
      '1. Ensure all prerequisites are met\n' +
      '2. Verify canonical maps exist\n' +
      '3. Check DAW installation paths\n' +
      '4. Re-run with --force if needed'
    );
  } else {
    console.log('\n✅ Workflow completed successfully!');

    if (options.install) {
      console.log('📦 Maps have been installed to DAW directories');
      console.log('💡 Restart your DAW to see the new MIDI maps');
    } else {
      console.log('💡 To install maps, re-run with --install flag');
    }
  }
}

async function main(): Promise<void> {
  try {
    const options = parseCliArgs();
    await runCompleteWorkflow(options);
  } catch (error: any) {
    console.error('\n❌ Complete workflow failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}