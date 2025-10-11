#!/usr/bin/env tsx
import { readdirSync, readFileSync, writeFileSync, unlinkSync } from 'fs';
import { join } from 'path';

interface PreJson {
  mode: string;
  tag: string;
  initialVersions: Record<string, string>;
  changesets: string[];
}

function ensureChangeset() {
  const rootDir = join(import.meta.dirname, '..');
  const changesetDir = join(rootDir, '.changeset');
  const preJsonPath = join(changesetDir, 'pre.json');

  // Read pre.json to see which changesets are tracked (unconsumed)
  let preJson: PreJson | null = null;
  try {
    const preJsonContent = readFileSync(preJsonPath, 'utf-8');
    preJson = JSON.parse(preJsonContent);
  } catch (err) {
    // No pre.json or not in prerelease mode - continue normally
  }

  // Get all .md files except README.md
  const allChangesetFiles = readdirSync(changesetDir).filter(
    f => f.endsWith('.md') && f !== 'README.md'
  );

  // If in prerelease mode, clean up consumed changesets
  if (preJson && preJson.changesets) {
    const trackedChangesets = new Set(preJson.changesets);
    const consumedChangesets: string[] = [];

    // Find changesets that exist as files but are NOT tracked in pre.json
    // These are consumed changesets that weren't cleaned up
    for (const file of allChangesetFiles) {
      const changesetName = file.replace('.md', '');
      if (!trackedChangesets.has(changesetName)) {
        consumedChangesets.push(file);
      }
    }

    // Delete consumed changeset files
    if (consumedChangesets.length > 0) {
      console.log(`Cleaning up ${consumedChangesets.length} consumed changeset(s)...`);
      for (const file of consumedChangesets) {
        const filePath = join(changesetDir, file);
        unlinkSync(filePath);
        console.log(`  ✓ Deleted ${file}`);
      }
    }
  }

  // Re-scan for remaining changesets after cleanup
  const remainingChangesets = readdirSync(changesetDir).filter(
    f => f.endsWith('.md') && f !== 'README.md'
  );

  if (remainingChangesets.length > 0) {
    console.log(`✓ Found ${remainingChangesets.length} unconsumed changeset(s), proceeding with release`);
    return;
  }

  // No unconsumed changesets - create automatic one
  console.log('No unconsumed changesets found, creating automatic changeset...');

  // Get current timestamp for changeset name
  const now = new Date();
  const timestamp = now.toISOString().replace(/[:.]/g, '-').split('T')[0];
  const timeStr = now.toISOString().split('T')[1].split('.')[0].replace(/:/g, '-');
  const changesetPath = join(changesetDir, `auto-release-${timestamp}-${timeStr}.md`);

  // Create changeset with all packages
  const changeset = `---
"@oletizi/audiotools": patch
"@oletizi/audiotools-config": patch
"@oletizi/sampler-lib": patch
"@oletizi/sampler-devices": patch
"@oletizi/sampler-midi": patch
"@oletizi/sampler-translate": patch
"@oletizi/sampler-export": patch
"@oletizi/sampler-backup": patch
"@oletizi/lib-runtime": patch
"@oletizi/lib-device-uuid": patch
"@oletizi/sampler-attic": patch
---

Automated release - ${now.toISOString()}
`;

  writeFileSync(changesetPath, changeset, 'utf-8');
  console.log(`✓ Created automatic changeset: ${changesetPath}`);
}

ensureChangeset();
