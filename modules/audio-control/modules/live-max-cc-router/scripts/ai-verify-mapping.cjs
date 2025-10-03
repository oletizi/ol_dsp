#!/usr/bin/env node

const fs = require('fs');
const { execSync } = require('child_process');
const yaml = require('yaml');

/**
 * Verify MIDI mappings using Claude Code CLI for semantic analysis
 *
 * Usage: node ai-verify-mapping.cjs <mapping.yaml> <descriptor.json>
 */

function generatePrompt(mappingPath, descriptorPath) {
  const mappingYaml = fs.readFileSync(mappingPath, 'utf8');
  const descriptorJson = fs.readFileSync(descriptorPath, 'utf8');

  const mapping = yaml.parse(mappingYaml);
  const descriptor = JSON.parse(descriptorJson);

  // Extract control names
  const controls = mapping.controls || [];
  const controlNames = [];

  function collectControl(ctrl) {
    if (!ctrl.plugin_parameter) return;
    const paramIndex = parseInt(ctrl.plugin_parameter);
    if (isNaN(paramIndex)) return; // Skip special params
    controlNames.push(ctrl.name);
  }

  controls.forEach(control => {
    if (control.buttons) {
      control.buttons.forEach(button => collectControl(button));
    } else {
      collectControl(control);
    }
  });

  // Format control list
  const controlList = controlNames.map((name, i) => `${i + 1}. ${name}`).join('\n');

  // Format parameter list
  const paramList = descriptor.parameters
    .map(p => `${p.index}: ${p.name}`)
    .join('\n');

  // Generate prompt
  const prompt = `I have a MIDI controller mapping for the ${mapping.plugin.manufacturer} ${mapping.plugin.name} plugin that needs semantic verification.

Below are two lists:
1. Control names from the MIDI mapping file
2. Plugin parameter names with their indices

Please match each control name to its best corresponding plugin parameter index. Consider:
- Common abbreviations (Comp = Compressor, Limit = Limiter, Thresh = Threshold)
- Synonyms (Dry/Wet = Mix, Enable = In, Shelf can refer to Pre-Low/Pre-High)
- Semantic equivalence (EQ Low Gain = Low Gain)

CONTROL NAMES:
${controlList}

PLUGIN PARAMETERS:
${paramList}

Please respond with ONLY a mapping table in this exact format (one line per control):
1 -> [index]
2 -> [index]
3 -> [index]
...

For example:
1 -> 4
2 -> 18
3 -> 1`;

  return {
    prompt,
    controlNames,
    paramsByIndex: descriptor.parameters.reduce((acc, p) => {
      acc[p.index] = p;
      return acc;
    }, {}),
    mapping,
    descriptor
  };
}

function parseClaudeResponse(response, controlNames, paramsByIndex) {
  const lines = response.trim().split('\n');
  const results = [];

  lines.forEach(line => {
    const match = line.match(/^(\d+)\s*->\s*(\d+)/);
    if (match) {
      const controlIdx = parseInt(match[1]) - 1;
      const paramIdx = parseInt(match[2]);

      if (controlIdx >= 0 && controlIdx < controlNames.length) {
        const controlName = controlNames[controlIdx];
        const param = paramsByIndex[paramIdx];

        results.push({
          control: controlName,
          paramIndex: paramIdx,
          paramName: param ? param.name : 'NOT FOUND'
        });
      }
    }
  });

  return results;
}

function displayResults(results, mapping) {
  console.log('\n╔══════════════════════════════════════════════════════════════════════════════════╗');
  console.log('║                    AI MAPPING VERIFICATION RESULTS                               ║');
  console.log('╚══════════════════════════════════════════════════════════════════════════════════╝\n');

  console.log(`Plugin: ${mapping.plugin.manufacturer} ${mapping.plugin.name}`);
  console.log(`Controller: ${mapping.device.manufacturer} ${mapping.device.model}\n`);

  console.log('─'.repeat(85));
  console.log(
    pad('Control Name', 35) +
    pad('Index', 8) +
    pad('Parameter Name', 40)
  );
  console.log('─'.repeat(85));

  results.forEach(r => {
    console.log(
      pad(r.control, 35) +
      pad(r.paramIndex, 8) +
      pad(r.paramName, 40)
    );
  });

  console.log('─'.repeat(85));
  console.log(`\n✓ Mapped ${results.length} controls\n`);
}

function pad(str, width) {
  const s = String(str);
  if (s.length >= width) {
    return s.substring(0, width - 1) + ' ';
  }
  return s + ' '.repeat(width - s.length);
}

// Main
if (require.main === module) {
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.error('Usage: node ai-verify-mapping.cjs <mapping.yaml> <descriptor.json>');
    console.error('\nExample:');
    console.error('  node ai-verify-mapping.cjs \\');
    console.error('    maps/novation-launch-control-xl-3/channel-strips/analog-obsession-channev.yaml \\');
    console.error('    plugin-descriptors/analogobsession-channev.json');
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

  console.log('🤖 Generating prompt for Claude Code...\n');
  const { prompt, controlNames, paramsByIndex, mapping } = generatePrompt(mappingPath, descriptorPath);

  // Write prompt to temp file
  const promptFile = '/tmp/claude-mapping-prompt.txt';
  fs.writeFileSync(promptFile, prompt, 'utf8');

  console.log('🚀 Calling Claude Code CLI...\n');

  try {
    // Find claude executable - try common paths
    const possiblePaths = [
      process.env.HOME + '/.claude/local/claude',
      '/usr/local/bin/claude',
      '/opt/homebrew/bin/claude',
      '/usr/bin/claude'
    ];

    let claudePath = null;
    for (const path of possiblePaths) {
      if (fs.existsSync(path)) {
        claudePath = path;
        break;
      }
    }

    if (!claudePath) {
      throw new Error('Claude CLI not found. Checked: ' + possiblePaths.join(', '));
    }

    console.log(`Using Claude at: ${claudePath}\n`);

    // Call Claude Code CLI
    const response = execSync(`"${claudePath}" "${prompt}"`, {
      encoding: 'utf8',
      maxBuffer: 10 * 1024 * 1024,
      stdio: ['pipe', 'pipe', 'inherit']
    });

    console.log('✨ Received response from Claude Code\n');

    // Parse response
    const results = parseClaudeResponse(response, controlNames, paramsByIndex);

    if (results.length === 0) {
      console.error('Error: Could not parse Claude Code response');
      console.error('Response was:');
      console.error(response);
      process.exit(1);
    }

    // Display results
    displayResults(results, mapping);

  } catch (error) {
    console.error('Error calling Claude Code CLI:', error.message);
    console.error('\nMake sure Claude Code CLI is installed and available as "claude" command.');
    console.error('You can install it from: https://github.com/anthropics/claude-code');
    process.exit(1);
  }
}

module.exports = { generatePrompt, parseClaudeResponse };
