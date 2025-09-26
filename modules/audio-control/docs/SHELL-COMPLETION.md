# Shell Completion for Audio Control Scripts

This document provides installation and usage instructions for shell completion of the 12 audio-control pnpm scripts.

## Overview

Shell completion provides auto-completion for:
- **12 workflow scripts**: `plugins:*`, `maps:*`, `daw:*`, `workflow:*`
- **Common flags**: `--help`, `--force`, `--install`, `--json`, `--verbose`, etc.
- **DAW targets**: `ardour`, `reaper`, `ableton`, `logic`
- **File paths**: For `--output` flag

## Supported Scripts

### Plugin Phase
- `pnpm plugins:extract [--force] [--verbose] [--json] [--output <dir>] [--dry-run]`
- `pnpm plugins:list [--json] [--verbose]`
- `pnpm plugins:health [--json] [--verbose] [--quiet]`

### Maps Phase
- `pnpm maps:validate [--verbose] [--json] [--quiet]`
- `pnpm maps:list [--json] [--verbose]`
- `pnpm maps:check [--verbose] [--json] [--quiet]`

### DAW Phase
- `pnpm daw:generate [--target <daw>] [--install] [--verbose] [--json] [--output <dir>] [--dry-run]`
- `pnpm daw:generate:ardour [--install] [--verbose] [--json] [--output <dir>] [--dry-run]`
- `pnpm daw:list [--json] [--verbose]`

### Workflow Phase
- `pnpm workflow:complete [--verbose] [--json] [--force] [--quiet]`
- `pnpm workflow:health [--verbose] [--json] [--force] [--quiet]`

## Installation

### Bash Completion

#### System-wide Installation (Recommended)

```bash
# Copy completion script to system directory
sudo cp scripts/completion/audio-control-completion.bash /etc/bash_completion.d/audio-control

# Reload bash completion
source /etc/bash_completion.d/audio-control
```

#### User-specific Installation

```bash
# Create user completion directory if it doesn't exist
mkdir -p ~/.local/share/bash-completion/completions

# Copy completion script
cp scripts/completion/audio-control-completion.bash ~/.local/share/bash-completion/completions/audio-control

# Add to ~/.bashrc if not already present
echo 'source ~/.local/share/bash-completion/completions/audio-control' >> ~/.bashrc

# Reload current session
source ~/.bashrc
```

#### macOS with Homebrew

```bash
# Install bash-completion if not already installed
brew install bash-completion

# Copy completion script
cp scripts/completion/audio-control-completion.bash $(brew --prefix)/etc/bash_completion.d/audio-control

# Add to ~/.bash_profile if not already present
echo 'source $(brew --prefix)/etc/bash_completion.d/audio-control' >> ~/.bash_profile

# Reload current session
source ~/.bash_profile
```

### Zsh Completion

#### Oh My Zsh Installation

```bash
# Copy completion script to Oh My Zsh completions directory
cp scripts/completion/audio-control-completion.zsh ~/.oh-my-zsh/completions/_audio-control

# Reload zsh completion
compinit
```

#### System-wide Installation

```bash
# Find zsh completion directory
echo $fpath

# Copy to a directory in fpath (typically /usr/local/share/zsh/site-functions)
sudo cp scripts/completion/audio-control-completion.zsh /usr/local/share/zsh/site-functions/_audio-control

# Reload zsh completion
compinit
```

#### User-specific Installation

```bash
# Create user completion directory
mkdir -p ~/.config/zsh/completions

# Copy completion script
cp scripts/completion/audio-control-completion.zsh ~/.config/zsh/completions/_audio-control

# Add to ~/.zshrc if not already present
echo 'fpath=(~/.config/zsh/completions $fpath)' >> ~/.zshrc
echo 'autoload -Uz compinit && compinit' >> ~/.zshrc

# Reload current session
source ~/.zshrc
```

## Usage Examples

### Basic Script Completion

```bash
# Type 'pnpm ' then press TAB to see all available scripts
pnpm <TAB>
# Shows: plugins:extract plugins:list plugins:health maps:validate ...

# Type partial script name and press TAB
pnpm plug<TAB>
# Completes to: pnpm plugins:

pnpm plugins:<TAB>
# Shows: plugins:extract plugins:list plugins:health
```

### Flag Completion

```bash
# Type script name followed by -- and press TAB
pnpm plugins:extract --<TAB>
# Shows: --force --help --verbose --json --output --dry-run

# Type partial flag and press TAB
pnpm daw:generate --tar<TAB>
# Completes to: pnpm daw:generate --target
```

### Target Completion

```bash
# After --target flag, press TAB to see available DAWs
pnpm daw:generate --target <TAB>
# Shows: ardour reaper ableton logic

# Type partial target and press TAB
pnpm daw:generate --target ar<TAB>
# Completes to: pnpm daw:generate --target ardour
```

### File Path Completion

```bash
# After --output flag, press TAB for directory completion
pnpm plugins:extract --output <TAB>
# Shows available directories in current path

# Navigate with TAB completion
pnpm plugins:extract --output /path/to/<TAB>
# Shows subdirectories under /path/to/
```

## Verification

### Test Bash Completion

```bash
# Test script completion
type 'pnpm plug' and press TAB - should show plugins: scripts

# Test flag completion
type 'pnpm plugins:extract --' and press TAB - should show available flags

# Test target completion
type 'pnpm daw:generate --target ' and press TAB - should show DAW options
```

### Test Zsh Completion

```bash
# Test with descriptions (zsh feature)
type 'pnpm ' and press TAB - should show scripts with descriptions

# Test context-aware completion
type 'pnpm daw:generate --' and press TAB - should show appropriate flags

# Verify completion function is loaded
which _audio_control_completion
# Should show the completion function location
```

## Troubleshooting

### Completion Not Working

1. **Verify completion is loaded**:
   ```bash
   # For bash
   complete -p pnpm
   # Should show: complete -F _audio_control_completion pnpm
   
   # For zsh
   which _audio_control_completion
   # Should show function definition
   ```

2. **Check file permissions**:
   ```bash
   ls -la scripts/completion/
   # Files should be readable (at least 644 permissions)
   ```

3. **Verify script syntax**:
   ```bash
   # For bash
   bash -n scripts/completion/audio-control-completion.bash
   # Should show no errors
   
   # For zsh
   zsh -n scripts/completion/audio-control-completion.zsh
   # Should show no errors
   ```

### Completion Shows Wrong Results

1. **Clear completion cache**:
   ```bash
   # For bash
   complete -r pnpm
   source /path/to/audio-control-completion.bash
   
   # For zsh
   unfunction _audio_control_completion
   compinit
   ```

2. **Check for conflicting completions**:
   ```bash
   # For bash
   complete | grep pnpm
   # Should only show audio-control completion
   
   # For zsh
   which _pnpm
   # Check if other pnpm completions exist
   ```

### Performance Issues

1. **Completion is slow**:
   - Large directories can slow file completion
   - Consider using `--output` with absolute paths
   - Check disk I/O if completion hangs

2. **Memory usage**:
   - Completion functions are lightweight
   - If issues persist, restart shell session

## Advanced Configuration

### Custom Aliases

If you use custom aliases for pnpm, add completion for them:

```bash
# For bash (add to completion script)
alias ac='pnpm'
complete -F _audio_control_completion ac

# For zsh (add to ~/.zshrc)
alias ac='pnpm'
compdef _audio_control_completion ac
```

### Integration with Package Managers

For npm users (if using npm instead of pnpm):

```bash
# The completion scripts already register for both pnpm and npm
# No additional configuration needed
```

### Custom Script Locations

If you've moved the completion scripts:

```bash
# Update paths in installation commands above
# Ensure scripts are in shell's completion search path
```

## Maintenance

### Updating Completions

When new scripts or flags are added:

1. Update completion scripts in `scripts/completion/`
2. Reinstall using instructions above
3. Test all completion scenarios

### Version Compatibility

- **Bash**: Requires bash 4.0+ for full functionality
- **Zsh**: Requires zsh 5.0+ for description support
- **macOS**: Default bash is 3.x, consider upgrading via Homebrew

## Contributing

### Adding New Script Completions

1. Add script name to `scripts` array in both completion files
2. Add specific flag handling in appropriate case statements
3. Update this documentation with new script details
4. Test completion functionality

### Testing Changes

```bash
# Test both shells
bash -c 'source scripts/completion/audio-control-completion.bash; complete -p pnpm'
zsh -c 'source scripts/completion/audio-control-completion.zsh; which _audio_control_completion'

# Test actual completion (manual verification required)
# Use test scenarios from Verification section above
```

Shell completion significantly improves the developer experience by reducing typing and preventing errors in script names and flags.