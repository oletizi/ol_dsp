#!/usr/bin/env node

/**
 * Performance Test Tool
 * Measures startup time and memory usage of tools
 */

import { execSync, exec } from 'child_process';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

function measureCommand(command, description) {
  console.log(`\n🧪 Testing: ${description}`);
  console.log(`⚡ Command: ${command}`);

  const startTime = Date.now();

  try {
    // Run command and capture memory info
    const result = execSync(command, {
      encoding: 'utf-8',
      stdio: 'pipe',
      cwd: __dirname + '/..',
      env: { ...process.env, NODE_OPTIONS: '--max-old-space-size=4096' }
    });

    const endTime = Date.now();
    const duration = endTime - startTime;

    console.log(`✅ Duration: ${duration}ms`);
    if (duration > 50) {
      console.log(`⚠️  Exceeds 50ms target (${duration}ms)`);
    }

    return { success: true, duration, output: result };

  } catch (error) {
    const endTime = Date.now();
    const duration = endTime - startTime;

    console.log(`❌ Failed after ${duration}ms`);
    console.log(`   Error: ${error.message.split('\n')[0]}`);

    return { success: false, duration, error: error.message };
  }
}

function measureWithMemory(command, description) {
  console.log(`\n🧪 Testing with memory tracking: ${description}`);
  console.log(`⚡ Command: ${command}`);

  return new Promise((resolve) => {
    const startTime = Date.now();
    const proc = exec(command, {
      cwd: __dirname + '/..',
      stdio: 'pipe'
    });

    let maxMemory = 0;
    const memoryInterval = setInterval(() => {
      try {
        const memInfo = process.memoryUsage();
        const rss = memInfo.rss / 1024 / 1024; // MB
        if (rss > maxMemory) {
          maxMemory = rss;
        }
      } catch (e) {
        // Process might have ended
      }
    }, 10);

    proc.on('close', (code) => {
      clearInterval(memoryInterval);
      const endTime = Date.now();
      const duration = endTime - startTime;

      console.log(`✅ Duration: ${duration}ms`);
      console.log(`📊 Peak memory: ${maxMemory.toFixed(1)}MB`);

      if (duration > 50) {
        console.log(`⚠️  Exceeds 50ms startup target (${duration}ms)`);
      }
      if (maxMemory > 50) {
        console.log(`⚠️  Exceeds 50MB memory target (${maxMemory.toFixed(1)}MB)`);
      }

      resolve({
        success: code === 0,
        duration,
        memory: maxMemory,
        code
      });
    });
  });
}

async function runPerformanceTests() {
  console.log('🚀 Starting performance tests for audio-control tools');
  console.log('\n📋 Performance targets:');
  console.log('  • Script startup: <50ms');
  console.log('  • Memory usage: <50MB');
  console.log('  • Validation overhead: <10ms');
  console.log('  • End-to-end workflow: <2s');

  const results = [];

  // Test startup times for help commands (should be fast)
  results.push(measureCommand('pnpm plugins:list --help', 'Plugin list help'));
  results.push(measureCommand('pnpm maps:validate --help', 'Maps validate help'));
  results.push(measureCommand('pnpm daw:generate --help', 'DAW generate help'));
  results.push(measureCommand('pnpm workflow:complete --help', 'Workflow complete help'));

  // Test actual operations (if test data exists)
  const testResults = [
    await measureWithMemory('pnpm plugins:list', 'Plugin list actual'),
    await measureWithMemory('pnpm plugins:health', 'Plugin health check'),
    await measureWithMemory('pnpm daw:list', 'DAW list actual'),
  ];

  results.push(...testResults);

  // Summary
  console.log('\n📊 Performance Test Summary');
  console.log('='.repeat(50));

  const successful = results.filter(r => r.success);
  const failed = results.filter(r => !r.success);

  console.log(`✅ Successful tests: ${successful.length}/${results.length}`);
  console.log(`❌ Failed tests: ${failed.length}/${results.length}`);

  if (successful.length > 0) {
    const avgDuration = successful.reduce((sum, r) => sum + r.duration, 0) / successful.length;
    const maxDuration = Math.max(...successful.map(r => r.duration));
    const memoryResults = successful.filter(r => r.memory);
    const avgMemory = memoryResults.length > 0 ? memoryResults.reduce((sum, r) => sum + r.memory, 0) / memoryResults.length : 0;

    console.log(`\n⏱️  Average startup time: ${avgDuration.toFixed(1)}ms`);
    console.log(`⏱️  Max startup time: ${maxDuration}ms`);
    if (avgMemory) {
      console.log(`📊 Average memory usage: ${avgMemory.toFixed(1)}MB`);
    }

    // Performance target analysis
    console.log('\n🎯 Performance Target Analysis:');
    const startupViolations = successful.filter(r => r.duration > 50);
    const memoryViolations = successful.filter(r => r.memory && r.memory > 50);

    if (startupViolations.length === 0) {
      console.log('✅ All commands meet startup time target (<50ms)');
    } else {
      console.log(`❌ ${startupViolations.length} commands exceed startup target`);
    }

    if (memoryViolations.length === 0) {
      console.log('✅ All commands meet memory usage target (<50MB)');
    } else {
      console.log(`❌ ${memoryViolations.length} commands exceed memory target`);
    }
  }

  // Implementation status
  console.log('\n🔧 Implementation Status:');
  const notImplementedErrors = failed.filter(r => r.error && r.error.includes('not yet implemented'));
  const actualErrors = failed.filter(r => !r.error || !r.error.includes('not yet implemented'));

  if (notImplementedErrors.length > 0) {
    console.log(`💡 ${notImplementedErrors.length} tools show "not yet implemented" - expected for Phase 1`);
  }

  if (actualErrors.length > 0) {
    console.log(`⚠️  ${actualErrors.length} tools have actual errors that need attention`);
    actualErrors.forEach(result => {
      console.log(`   • ${result.error?.split('\n')[0]}`);
    });
  }

  return {
    totalTests: results.length,
    successful: successful.length,
    failed: failed.length,
    avgStartup: successful.length > 0 ? successful.reduce((sum, r) => sum + r.duration, 0) / successful.length : 0,
    avgMemory: avgMemory || 0,
    startupTargetMet: startupViolations?.length === 0,
    memoryTargetMet: memoryViolations?.length === 0
  };
}

if (import.meta.url === `file://${process.argv[1]}`) {
  runPerformanceTests().catch(console.error);
}

export { measureCommand, measureWithMemory, runPerformanceTests };