#!/usr/bin/env node

/**
 * CLI Optimizer
 * Fast command dispatcher to reduce startup times
 * Targets: <50ms script startup
 */

import { spawn } from 'child_process';
import { resolve, dirname } from 'path';
import { fileURLToPath } from 'url';
import { existsSync } from 'fs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Command mapping for fast dispatch
const COMMANDS = {
  // Plugin phase
  'plugins:extract': 'plugins/extract.ts',
  'plugins:list': 'plugins/list.ts',
  'plugins:health': 'plugins/health.ts',

  // Maps phase
  'maps:validate': 'maps/validate.ts',
  'maps:list': 'maps/list.ts',
  'maps:check': 'maps/check.ts',

  // DAW phase
  'daw:generate': 'daw/generate.ts',
  'daw:list': 'daw/list.ts',

  // Workflow phase
  'workflow:complete': 'workflow/complete.ts',
  'workflow:health': 'workflow/health.ts'
};

function showHelp() {
  console.log('Audio Control CLI Optimizer');
  console.log('Fast command dispatcher for audio-control tools');
  console.log('');
  console.log('Usage: node cli-optimizer.js <command> [args...]');
  console.log('');
  console.log('Available commands:');

  Object.keys(COMMANDS).forEach(cmd => {
    console.log(`  ${cmd.padEnd(20)} - ${getCommandDescription(cmd)}`);
  });

  console.log('');
  console.log('Examples:');
  console.log('  node cli-optimizer.js plugins:list --help');
  console.log('  node cli-optimizer.js maps:validate ./maps');
  console.log('  node cli-optimizer.js workflow:complete --target ardour');
}

function getCommandDescription(command) {
  const descriptions = {
    'plugins:extract': 'Extract plugin parameter information',
    'plugins:list': 'List available plugins',
    'plugins:health': 'Check plugin extraction health',
    'maps:validate': 'Validate canonical mapping files',
    'maps:list': 'List available canonical mappings',
    'maps:check': 'Check mapping compatibility',
    'daw:generate': 'Generate DAW-specific maps',
    'daw:list': 'List generated DAW maps',
    'workflow:complete': 'Run complete workflow',
    'workflow:health': 'Check workflow health'
  };

  return descriptions[command] || 'Audio control tool';
}

function optimizeExecution(scriptPath, args) {
  return new Promise((resolve, reject) => {
    const startTime = Date.now();

    // Use tsx for TypeScript execution - this is still the bottleneck
    // but we can optimize the environment
    const child = spawn('tsx', [scriptPath, ...args], {
      stdio: 'inherit',
      cwd: resolve(__dirname, '..'),
      env: {
        ...process.env,
        // Optimize Node.js for faster startup
        NODE_OPTIONS: '--max-old-space-size=1024 --no-warnings',
        // Disable TSX features we don't need for faster startup
        TSX_TSCONFIG_PATH: resolve(__dirname, '../tsconfig.base.json')
      }
    });

    child.on('close', (code) => {
      const duration = Date.now() - startTime;

      if (duration > 100) {
        console.warn(`⚠️  Command took ${duration}ms (target: <50ms)`);
      }

      resolve(code);
    });

    child.on('error', (error) => {
      reject(error);
    });
  });
}

async function main() {
  const args = process.argv.slice(2);

  if (args.length === 0 || args[0] === '--help' || args[0] === '-h') {
    showHelp();
    process.exit(0);
  }

  const command = args[0];
  const commandArgs = args.slice(1);

  // Check if command exists
  if (!COMMANDS[command]) {
    console.error(`❌ Unknown command: ${command}`);
    console.error('');
    console.error('Available commands:');
    Object.keys(COMMANDS).forEach(cmd => {
      console.log(`  ${cmd}`);
    });
    process.exit(1);
  }

  // Build script path
  const scriptPath = resolve(__dirname, COMMANDS[command]);

  if (!existsSync(scriptPath)) {
    console.error(`❌ Script not found: ${scriptPath}`);
    process.exit(1);
  }

  try {
    const exitCode = await optimizeExecution(scriptPath, commandArgs);
    process.exit(exitCode || 0);
  } catch (error) {
    console.error(`❌ Execution failed:`, error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main().catch(console.error);
}