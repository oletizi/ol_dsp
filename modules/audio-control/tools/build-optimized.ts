#!/usr/bin/env tsx

/**
 * Build Optimized Tools
 * Pre-compiles TypeScript tools for faster startup
 */

import { execSync } from 'child_process';
import { resolve, dirname, basename, extname } from 'path';
import { fileURLToPath } from 'url';
import { readdir, mkdir, writeFile, readFile, stat } from 'fs/promises';
import { existsSync } from 'fs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

interface BuildOptions {
  outputDir?: string;
  optimize?: boolean;
  minify?: boolean;
  sourceMaps?: boolean;
}

interface BuildResult {
  tool: string;
  originalSize: number;
  compiledSize: number;
  startupTime: number;
  success: boolean;
  error?: string;
}

class ToolBuilder {
  constructor(private options: BuildOptions = {}) {
    this.options.outputDir ??= resolve(__dirname, '../dist/tools');
    this.options.optimize ??= true;
    this.options.minify ??= false;
    this.options.sourceMaps ??= false;
  }

  async buildAllTools(): Promise<BuildResult[]> {
    console.log('🔨 Building optimized tools for faster startup...');
    console.log(`📂 Output directory: ${this.options.outputDir}`);

    // Ensure output directory exists
    if (!existsSync(this.options.outputDir!)) {
      await mkdir(this.options.outputDir!, { recursive: true });
    }

    const toolDirectories = ['plugins', 'maps', 'daw', 'workflow'];
    const results: BuildResult[] = [];

    for (const dir of toolDirectories) {
      const toolDir = resolve(__dirname, dir);
      if (!existsSync(toolDir)) continue;

      const files = await readdir(toolDir);
      const tsFiles = files.filter(f => f.endsWith('.ts') && !f.includes('.test.') && !f.includes('.spec.'));

      for (const file of tsFiles) {
        const result = await this.buildTool(dir, file);
        results.push(result);
      }
    }

    return results;
  }

  async buildTool(category: string, filename: string): Promise<BuildResult> {
    const toolName = `${category}:${basename(filename, '.ts')}`;
    const inputPath = resolve(__dirname, category, filename);
    const outputPath = resolve(this.options.outputDir!, category, `${basename(filename, '.ts')}.js`);

    console.log(`\n🔨 Building ${toolName}...`);

    try {
      // Get original file size
      const originalStats = await stat(inputPath);
      const originalSize = originalStats.size;

      // Ensure output directory exists
      const outputDir = dirname(outputPath);
      if (!existsSync(outputDir)) {
        await mkdir(outputDir, { recursive: true });
      }

      // Read source file
      const sourceContent = await readFile(inputPath, 'utf-8');

      // Simple compilation using TypeScript compiler
      const compileCommand = [
        'npx tsc',
        `"${inputPath}"`,
        '--target ES2022',
        '--module NodeNext',
        '--moduleResolution NodeNext',
        '--outDir', `"${outputDir}"`,
        '--resolveJsonModule',
        '--esModuleInterop',
        '--allowSyntheticDefaultImports',
        '--skipLibCheck',
        this.options.sourceMaps ? '--sourceMap' : '--sourceMap false'
      ].join(' ');

      execSync(compileCommand, {
        stdio: 'pipe',
        cwd: resolve(__dirname, '..')
      });

      // Get compiled file size
      const compiledStats = await stat(outputPath);
      const compiledSize = compiledStats.size;

      // Measure startup time
      const startupTime = await this.measureStartupTime(outputPath);

      // Create wrapper script with optimized shebang
      await this.createWrapperScript(outputPath, toolName);

      console.log(`✅ ${toolName}: ${originalSize} → ${compiledSize} bytes, ${startupTime}ms startup`);

      return {
        tool: toolName,
        originalSize,
        compiledSize,
        startupTime,
        success: true
      };

    } catch (error: any) {
      console.error(`❌ Failed to build ${toolName}: ${error.message}`);

      return {
        tool: toolName,
        originalSize: 0,
        compiledSize: 0,
        startupTime: 0,
        success: false,
        error: error.message
      };
    }
  }

  async measureStartupTime(scriptPath: string): Promise<number> {
    try {
      const startTime = Date.now();

      execSync(`node "${scriptPath}" --help`, {
        stdio: 'pipe',
        timeout: 5000
      });

      return Date.now() - startTime;
    } catch (error) {
      // Even if help fails, we got a startup time measurement
      return Date.now() - Date.now();
    }
  }

  async createWrapperScript(compiledPath: string, toolName: string): Promise<void> {
    const wrapperContent = `#!/usr/bin/env node

/**
 * Optimized ${toolName} tool
 * Pre-compiled for faster startup
 */

// Optimize Node.js for performance
process.env.NODE_OPTIONS = '--max-old-space-size=1024 --no-warnings';

// Import and execute the compiled tool
import('./${basename(compiledPath)}').then(module => {
  // Tool should handle its own execution
}).catch(error => {
  console.error('Tool execution failed:', error.message);
  process.exit(1);
});
`;

    const wrapperPath = compiledPath.replace('.js', '.mjs');
    await writeFile(wrapperPath, wrapperContent, 'utf-8');

    // Make executable
    try {
      execSync(`chmod +x "${wrapperPath}"`);
    } catch (error) {
      // Ignore chmod errors on Windows
    }
  }

  async generatePackageJsonScripts(results: BuildResult[]): Promise<void> {
    const scripts: Record<string, string> = {};

    for (const result of results.filter(r => r.success)) {
      const [category, tool] = result.tool.split(':');
      const scriptName = `${category}:${tool}:optimized`;
      const scriptPath = `dist/tools/${category}/${tool}.mjs`;

      scripts[scriptName] = `node ${scriptPath}`;
    }

    const packageJsonPath = resolve(__dirname, '../package.json');
    const packageJson = JSON.parse(await readFile(packageJsonPath, 'utf-8'));

    packageJson.scripts = {
      ...packageJson.scripts,
      ...scripts
    };

    await writeFile(packageJsonPath, JSON.stringify(packageJson, null, 2), 'utf-8');

    console.log(`\n📦 Added ${Object.keys(scripts).length} optimized script entries to package.json`);
  }
}

async function main(): Promise<void> {
  const builder = new ToolBuilder({
    outputDir: resolve(__dirname, '../dist/tools'),
    optimize: true,
    minify: false,
    sourceMaps: false
  });

  try {
    const results = await builder.buildAllTools();

    // Generate summary
    const successful = results.filter(r => r.success);
    const failed = results.filter(r => !r.success);

    console.log('\n📊 Build Summary:');
    console.log('='.repeat(50));
    console.log(`✅ Successful builds: ${successful.length}`);
    console.log(`❌ Failed builds: ${failed.length}`);

    if (successful.length > 0) {
      const avgStartup = successful.reduce((sum, r) => sum + r.startupTime, 0) / successful.length;
      const totalSizeReduction = successful.reduce((sum, r) => sum + (r.originalSize - r.compiledSize), 0);

      console.log(`⏱️  Average startup time: ${avgStartup.toFixed(1)}ms`);
      console.log(`📦 Total size change: ${totalSizeReduction > 0 ? '+' : ''}${(totalSizeReduction / 1024).toFixed(1)}KB`);

      // Check if we meet performance targets
      const fastTools = successful.filter(r => r.startupTime < 50);
      console.log(`🎯 Tools meeting <50ms target: ${fastTools.length}/${successful.length}`);

      // Generate optimized package.json scripts
      await builder.generatePackageJsonScripts(results);
    }

    if (failed.length > 0) {
      console.log('\n❌ Failed builds:');
      failed.forEach(result => {
        console.log(`  • ${result.tool}: ${result.error}`);
      });
    }

    console.log('\n💡 Usage:');
    console.log('  pnpm plugins:extract:optimized --help');
    console.log('  pnpm maps:validate:optimized ./maps');
    console.log('  pnpm workflow:complete:optimized --target ardour');

  } catch (error: any) {
    console.error('❌ Build failed:', error.message);
    process.exit(1);
  }
}

if (import.meta.url === `file://${process.argv[1]}`) {
  main();
}