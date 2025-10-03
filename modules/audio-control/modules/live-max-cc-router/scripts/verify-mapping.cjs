#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const yaml = require('yaml');
const Anthropic = require('@anthropic-ai/sdk');

/**
 * Verify canonical MIDI mappings against plugin descriptors using Claude AI
 *
 * Usage: node verify-mapping.cjs <mapping.yaml> <descriptor.json>
 */

// Initialize Anthropic client
const anthropic = new Anthropic({
  apiKey: process.env.ANTHROPIC_API_KEY,
});

async function verifyMapping(mappingPath, descriptorPath) {
  // Read files
  const mappingYaml = fs.readFileSync(mappingPath, 'utf8');
  const descriptorJson = fs.readFileSync(descriptorPath, 'utf8');

  const mapping = yaml.parse(mappingYaml);
  const descriptor = JSON.parse(descriptorJson);

  console.log('\n╔══════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                    CANONICAL MAPPING VERIFICATION (AI)                           ║');
  console.log('╚══════════════════════════════════════════════════════════════════════════════════╝\n');

  console.log(`Plugin: ${mapping.plugin.manufacturer} ${mapping.plugin.name}`);
  console.log(`Controller: ${mapping.device.manufacturer} ${mapping.device.model}`);
  console.log(`Mapping: ${mapping.metadata.name}\n`);

  // Create parameter lookup by index
  const paramsByIndex = {};
  descriptor.parameters.forEach(param => {
    paramsByIndex[param.index] = param;
  });

  // Collect all control-parameter pairs for AI analysis
  const controls = mapping.controls || [];
  const controlData = [];

  function collectControl(ctrl) {
    if (!ctrl.plugin_parameter) {
      return; // Skip unassigned controls
    }

    const cc = ctrl.cc;
    const mappingName = ctrl.name;
    const paramIndex = parseInt(ctrl.plugin_parameter);

    // Special cases for non-numeric parameter references
    if (isNaN(paramIndex)) {
      controlData.push({
        cc,
        mappingName,
        paramIndex: '--',
        pluginName: `[${ctrl.plugin_parameter}]`,
        status: 'SPECIAL'
      });
      return;
    }

    const pluginParam = paramsByIndex[paramIndex];

    if (!pluginParam) {
      controlData.push({
        cc,
        mappingName,
        paramIndex,
        pluginName: 'NOT FOUND',
        status: 'MISSING'
      });
      return;
    }

    controlData.push({
      cc,
      mappingName,
      paramIndex,
      pluginName: pluginParam.name,
      status: 'PENDING' // Will be determined by AI
    });
  }

  controls.forEach(control => {
    if (control.buttons) {
      control.buttons.forEach(button => {
        collectControl(button);
      });
    } else {
      collectControl(control);
    }
  });

  // Batch check names with Claude
  console.log('🤖 Analyzing name matches with Claude AI...\n');
  await batchCheckNames(controlData);

  // Display results
  console.log('─'.repeat(100));
  console.log(
    pad('CC', 5) +
    pad('Control Name (Mapping)', 35) +
    pad('Idx', 5) +
    pad('Parameter Name (Plugin)', 35) +
    pad('Status', 15)
  );
  console.log('─'.repeat(100));

  let mismatches = 0;
  let matches = 0;
  let missing = 0;
  let special = 0;

  controlData.forEach(data => {
    const { cc, mappingName, paramIndex, pluginName, status } = data;

    let statusStr;
    if (status === 'MATCH') {
      statusStr = green(pad('✓ OK', 15));
      matches++;
    } else if (status === 'MISMATCH') {
      statusStr = red(pad('⚠ MISMATCH', 15));
      mismatches++;
    } else if (status === 'MISSING') {
      statusStr = red(pad('MISSING', 15));
      missing++;
    } else if (status === 'SPECIAL') {
      statusStr = yellow(pad('SPECIAL', 15));
      special++;
    }

    console.log(
      pad(cc, 5) +
      pad(mappingName, 35) +
      pad(paramIndex, 5) +
      pad(pluginName, 35) +
      statusStr
    );
  });

  console.log('─'.repeat(100));
  console.log('\n📊 Summary:');
  console.log(`   ${green('✓')} Matches:    ${matches}`);
  console.log(`   ${red('⚠')} Mismatches: ${mismatches}`);
  console.log(`   ${red('✗')} Missing:    ${missing}`);
  console.log(`   ${yellow('⚡')} Special:    ${special}`);
  console.log(`   Total mapped controls: ${matches + mismatches + missing + special}\n`);

  if (mismatches > 0 || missing > 0) {
    console.log(`${red('❌ Verification failed')}: ${mismatches + missing} issues found\n`);
    process.exit(1);
  } else {
    console.log(`${green('✅ Verification passed')}: All mappings are correct\n`);
  }
}

// Batch check names using Claude AI
async function batchCheckNames(controlData) {
  // Filter items that need AI checking
  const itemsToCheck = controlData.filter(d => d.status === 'PENDING');

  if (itemsToCheck.length === 0) {
    return;
  }

  // Build comparison list for Claude
  const comparisons = itemsToCheck.map((d, i) =>
    `${i + 1}. Control: "${d.mappingName}" vs Plugin: "${d.pluginName}"`
  ).join('\n');

  const prompt = `You are analyzing MIDI controller mappings for an audio plugin. I need to determine if control names from a MIDI mapping file semantically match the actual plugin parameter names.

For each pair below, respond with ONLY "MATCH" or "MISMATCH" (one per line, in order).

Consider these as MATCH:
- Exact semantic equivalents (e.g., "Comp Dry/Wet" = "Compressor Mix")
- Common abbreviations (e.g., "Comp" = "Compressor", "Limit" = "Limiter", "Thresh" = "Threshold")
- Synonyms for the same control (e.g., "Dry/Wet" = "Mix", "Enable" = "In")
- Different word order for same meaning (e.g., "EQ Low Gain" = "Low Gain")

Consider these as MISMATCH:
- Different parameters entirely (e.g., "Low Freq" vs "High Freq")
- Wrong section (e.g., "Compressor X" vs "Limiter X")

Pairs to analyze:
${comparisons}

Respond with exactly ${itemsToCheck.length} lines, each containing only "MATCH" or "MISMATCH":`;

  try {
    const message = await anthropic.messages.create({
      model: 'claude-sonnet-4-20250514',
      max_tokens: 1024,
      messages: [{
        role: 'user',
        content: prompt
      }]
    });

    const responseText = message.content[0].text.trim();
    const results = responseText.split('\n').map(line => line.trim().toUpperCase());

    // Apply results back to controlData
    itemsToCheck.forEach((data, i) => {
      const result = results[i];
      if (result === 'MATCH') {
        data.status = 'MATCH';
      } else if (result === 'MISMATCH') {
        data.status = 'MISMATCH';
      } else {
        // Fallback if AI response is malformed
        data.status = 'MISMATCH';
        console.warn(`Warning: Unexpected AI response for item ${i + 1}: "${result}"`);
      }
    });
  } catch (error) {
    console.error('Error calling Claude API:', error.message);
    console.error('Falling back to all MISMATCH for safety\n');
    itemsToCheck.forEach(data => {
      data.status = 'MISMATCH';
    });
  }
}

// Helper: Pad string to width
function pad(str, width) {
  const s = String(str);
  if (s.length >= width) {
    return s.substring(0, width - 1) + ' ';
  }
  return s + ' '.repeat(width - s.length);
}

// ANSI color helpers
function red(str) {
  return `\x1b[31m${str}\x1b[0m`;
}

function green(str) {
  return `\x1b[32m${str}\x1b[0m`;
}

function yellow(str) {
  return `\x1b[33m${str}\x1b[0m`;
}

// Main
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.error('Usage: node verify-mapping.cjs <mapping.yaml> <descriptor.json>');
    console.error('\nExample:');
    console.error('  node verify-mapping.cjs \\');
    console.error('    maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml \\');
    console.error('    plugin-descriptors/analogobsession-channev.json');
    console.error('\nNote: Requires ANTHROPIC_API_KEY environment variable');
    process.exit(1);
  }

  const [mappingPath, descriptorPath] = args;

  if (!fs.existsSync(mappingPath)) {
    console.error(`Error: Mapping file not found: ${mappingPath}`);
    process.exit(1);
  }

  if (!fs.existsSync(descriptorPath)) {
    console.error(`Error: Descriptor file not found: ${descriptorPath}`);
    process.exit(1);
  }

  if (!process.env.ANTHROPIC_API_KEY) {
    console.error('Error: ANTHROPIC_API_KEY environment variable not set');
    process.exit(1);
  }

  verifyMapping(mappingPath, descriptorPath)
    .catch(err => {
      console.error('Fatal error:', err);
      process.exit(1);
    });
}

module.exports = { verifyMapping };
