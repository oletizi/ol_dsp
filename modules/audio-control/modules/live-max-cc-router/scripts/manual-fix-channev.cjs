#!/usr/bin/env node

const fs = require('fs');
const yaml = require('yaml');

/**
 * Manual corrections for CHANNEV mapping based on parameter analysis
 */

const MANUAL_MAPPINGS = {
  // High pass filters
  "High Pass": 4,           // Pre-Hipass (pre-EQ high pass)
  "EQ High Pass": 18,       // Eq-Hipass (EQ section high pass)

  // Compressor controls
  "Comp Dry/Wet": 24,       // Compressor Mix
  "Comp Ratio": 21,         // Compressor Ratio
  "Comp Threshold": 20,     // Compressor Threshold
  "Comp Release": 22,       // Compressor Release
  "Comp Gain": 23,          // Compressor Gain
  "Comp Enable": 43,        // Compressor In

  // Limiter controls
  "Limit Mix": 28,          // Limiter Mix
  "Limit Threshold": 25,    // Limiter Threshold
  "Limit Release": 26,      // Limiter Release
  "Limit Gain": 27,         // Limiter Gain
  "Limit Enable": 45,       // Limiter In

  // EQ controls
  "Low Shelf": 2,           // Pre-Low (pre-EQ low shelf)
  "High Shelf": 3,          // Pre-High (pre-EQ high shelf)
  "EQ Low-Mid Freq": 13,    // Low Mid Freq
  "EQ Low-Mid Gain": 12,    // Low Mid Gain
  "EQ Low-Mid Q": 39,       // Low Mid Q
  "EQ High-Mid Freq": 15,   // High Mid Freq
  "EQ High-Mid Gain": 14,   // High Mid Gain
  "EQ High-Mid Q": 40,      // High Mid Q
  "EQ Enable": 42,          // Equalizer In
};

const mappingPath = process.argv[2] ||
  '../../canonical-midi-maps/maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml';

const fullPath = require('path').resolve(__dirname, mappingPath);

console.log(`\n🔧 Applying manual CHANNEV mapping corrections...\n`);

const mappingYaml = fs.readFileSync(fullPath, 'utf8');
const mapping = yaml.parse(mappingYaml);

let fixed = 0;

function processControl(ctrl) {
  if (!ctrl.plugin_parameter || !ctrl.name) return;

  const manualIndex = MANUAL_MAPPINGS[ctrl.name];
  if (manualIndex !== undefined) {
    const oldIndex = ctrl.plugin_parameter;
    ctrl.plugin_parameter = manualIndex.toString();
    console.log(`  ✓ ${ctrl.name}: ${oldIndex} → ${manualIndex}`);
    fixed++;
  }
}

mapping.controls.forEach(control => {
  if (control.buttons) {
    control.buttons.forEach(processControl);
  } else {
    processControl(control);
  }
});

const updatedYaml = yaml.stringify(mapping, {
  lineWidth: 0,
  defaultStringType: 'QUOTE_DOUBLE',
});

fs.writeFileSync(fullPath, updatedYaml, 'utf8');

console.log(`\n✅ Fixed ${fixed} mappings\n`);
console.log(`Updated: ${fullPath}\n`);
