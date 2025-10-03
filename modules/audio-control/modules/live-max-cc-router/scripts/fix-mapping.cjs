#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const yaml = require('yaml');

/**
 * Auto-fix canonical MIDI mappings using plugin descriptor
 *
 * Matches control names from existing mapping to parameter names in descriptor
 * and updates the plugin_parameter indices to be correct.
 *
 * Usage: node fix-mapping.cjs <mapping.yaml> <descriptor.json> [output.yaml]
 */

function fixMapping(mappingPath, descriptorPath, outputPath) {
  // Read files
  const mappingYaml = fs.readFileSync(mappingPath, 'utf8');
  const descriptorJson = fs.readFileSync(descriptorPath, 'utf8');

  const mapping = yaml.parse(mappingYaml);
  const descriptor = JSON.parse(descriptorJson);

  console.log('\n╔══════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                    CANONICAL MAPPING AUTO-FIX                                    ║');
  console.log('╚══════════════════════════════════════════════════════════════════════════════════╝\n');

  console.log(`Plugin: ${mapping.plugin.manufacturer} ${mapping.plugin.name}`);
  console.log(`Controller: ${mapping.device.manufacturer} ${mapping.device.model}`);
  console.log(`Mapping: ${mapping.metadata.name}\n`);

  // Create parameter lookup by name (normalized)
  const paramsByNormalizedName = {};
  const paramsByIndex = {};
  descriptor.parameters.forEach(param => {
    const normalized = normalizeName(param.name);
    paramsByNormalizedName[normalized] = param;
    paramsByIndex[param.index] = param;
  });

  // Stats
  let fixed = 0;
  let unchanged = 0;
  let notFound = 0;
  let special = 0;

  // Process all controls
  const controls = mapping.controls || [];

  function processControl(ctrl) {
    if (!ctrl.plugin_parameter) {
      return; // Skip unassigned controls
    }

    const oldIndex = ctrl.plugin_parameter;

    // Skip special parameters
    if (isNaN(parseInt(oldIndex))) {
      console.log(yellow(`  SPECIAL: ${ctrl.name} -> [${oldIndex}]`));
      special++;
      return;
    }

    // Find best matching parameter by name
    const bestMatch = findBestMatch(ctrl.name, descriptor.parameters);

    if (!bestMatch) {
      console.log(red(`  NOT FOUND: ${ctrl.name} (was index ${oldIndex})`));
      notFound++;
      return;
    }

    const newIndex = bestMatch.index.toString();

    if (oldIndex === newIndex) {
      console.log(green(`  ✓ OK: ${ctrl.name} -> ${bestMatch.name} [${newIndex}]`));
      unchanged++;
    } else {
      console.log(cyan(`  FIXED: ${ctrl.name} -> ${bestMatch.name} [${oldIndex} → ${newIndex}]`));
      ctrl.plugin_parameter = newIndex;
      fixed++;
    }
  }

  controls.forEach(control => {
    // Handle button groups
    if (control.buttons) {
      control.buttons.forEach(button => {
        processControl(button);
      });
    } else {
      processControl(control);
    }
  });

  console.log('\n' + '─'.repeat(100));
  console.log('\n📊 Summary:');
  console.log(`   ${cyan('🔧')} Fixed:     ${fixed}`);
  console.log(`   ${green('✓')} Unchanged: ${unchanged}`);
  console.log(`   ${yellow('⚡')} Special:   ${special}`);
  console.log(`   ${red('✗')} Not found: ${notFound}`);
  console.log(`   Total mapped controls: ${fixed + unchanged + special + notFound}\n`);

  if (notFound > 0) {
    console.log(yellow(`⚠️  Warning: ${notFound} controls could not be matched to plugin parameters`));
  }

  if (fixed > 0 || notFound > 0) {
    // Write output
    const output = outputPath || mappingPath;
    const updatedYaml = yaml.stringify(mapping, {
      lineWidth: 0,
      defaultStringType: 'QUOTE_DOUBLE',
    });

    fs.writeFileSync(output, updatedYaml, 'utf8');
    console.log(green(`\n✅ Updated mapping written to: ${output}\n`));
  } else {
    console.log(green('\n✅ No changes needed - all mappings are already correct!\n'));
  }
}

// Find best matching parameter by fuzzy name matching
function findBestMatch(controlName, parameters) {
  const normalizedControl = normalizeName(controlName);

  // First try exact match
  for (const param of parameters) {
    if (normalizeName(param.name) === normalizedControl) {
      return param;
    }
  }

  // Try substring match
  for (const param of parameters) {
    const normalizedParam = normalizeName(param.name);
    if (normalizedParam.includes(normalizedControl) || normalizedControl.includes(normalizedParam)) {
      return param;
    }
  }

  // Try with abbreviation expansion
  const expandedControl = expandAbbreviations(normalizedControl);
  for (const param of parameters) {
    const expandedParam = expandAbbreviations(normalizeName(param.name));
    if (expandedParam === expandedControl) {
      return param;
    }
    if (expandedParam.includes(expandedControl) || expandedControl.includes(expandedParam)) {
      return param;
    }
  }

  // Try word-by-word matching (for multi-word controls)
  const controlWords = normalizedControl.split(/\s+/);
  const candidates = parameters.map(param => {
    const paramWords = normalizeName(param.name).split(/\s+/);
    const matchScore = controlWords.filter(word => paramWords.some(pw => pw.includes(word) || word.includes(pw))).length;
    return { param, score: matchScore };
  });

  const bestCandidate = candidates.reduce((best, curr) => curr.score > best.score ? curr : best, { score: 0 });

  if (bestCandidate.score > 0) {
    return bestCandidate.param;
  }

  return null;
}

// Normalize name for comparison
function normalizeName(str) {
  return str
    .toLowerCase()
    .replace(/[^a-z0-9\s]/g, '')
    .replace(/\s+/g, ' ')
    .trim();
}

// Expand common abbreviations
function expandAbbreviations(str) {
  const abbrevs = {
    'comp': 'compressor',
    'thresh': 'threshold',
    'eq': 'equalizer',
    'freq': 'frequency',
    'mic': 'microphone',
    'pre': 'preamp',
    'hipass': 'highpass',
    'lopass': 'lowpass',
    'hpf': 'highpass',
    'lpf': 'lowpass',
  };

  let result = str;
  Object.entries(abbrevs).forEach(([abbr, full]) => {
    result = result.replace(new RegExp(`\\b${abbr}\\b`, 'g'), full);
  });

  return result;
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

function cyan(str) {
  return `\x1b[36m${str}\x1b[0m`;
}

// Main
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.error('Usage: node fix-mapping.cjs <mapping.yaml> <descriptor.json> [output.yaml]');
    console.error('\nExample:');
    console.error('  node fix-mapping.cjs \\');
    console.error('    maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml \\');
    console.error('    plugin-descriptors/analogobsession-channev.json');
    console.error('\nIf output.yaml is not specified, the original mapping file will be updated.');
    process.exit(1);
  }

  const [mappingPath, descriptorPath, outputPath] = args;

  if (!fs.existsSync(mappingPath)) {
    console.error(`Error: Mapping file not found: ${mappingPath}`);
    process.exit(1);
  }

  if (!fs.existsSync(descriptorPath)) {
    console.error(`Error: Descriptor file not found: ${descriptorPath}`);
    process.exit(1);
  }

  fixMapping(mappingPath, descriptorPath, outputPath);
}

module.exports = { fixMapping };
