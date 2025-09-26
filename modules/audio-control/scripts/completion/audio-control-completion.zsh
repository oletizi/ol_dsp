#compdef pnpm
# audio-control zsh completion script
# Generated for audio-control monorepo project

_audio_control_completion() {
    local context state state_descr line
    typeset -A opt_args
    
    local scripts=(
        'plugins:extract:Extract plugin descriptors from available plugins'
        'plugins:list:List available plugins and their status'
        'plugins:health:Validate extracted plugin descriptors'
        'maps:validate:Validate canonical mapping files'
        'maps:list:List available canonical mappings'
        'maps:check:Health check mappings against descriptors'
        'daw:generate:Generate all DAW formats'
        'daw:generate:ardour:Generate Ardour mappings only'
        'daw:list:List generated DAW files'
        'workflow:complete:Run complete workflow (extract → validate → generate)'
        'workflow:health:System health check across all phases'
    )
    
    local common_flags=(
        '--help[Show help information]'
        '--force[Force operation, bypass cache/confirmations]'
        '--install[Install generated files to target location]'
        '--json[Output results in JSON format]'
        '--verbose[Enable verbose output]'
        '--target[Specify target DAW or format]:target:(ardour reaper ableton logic)'
        '--output[Specify output directory]:output:_directories'
        '--dry-run[Show what would be done without executing]'
        '--quiet[Suppress non-essential output]'
    )
    
    _arguments -C \
        '1: :->scripts' \
        '*: :->flags' && return 0
    
    case $state in
        scripts)
            _describe 'audio-control scripts' scripts
            ;;
        flags)
            case $words[2] in
                plugins:extract)
                    _arguments \
                        '--force[Force re-extraction, bypass cache]' \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--output[Specify output directory]:output:_directories' \
                        '--dry-run[Show what would be done without executing]'
                    ;;
                plugins:list)
                    _arguments \
                        '--help[Show help information]' \
                        '--json[Output results in JSON format]' \
                        '--verbose[Enable verbose output]'
                    ;;
                plugins:health)
                    _arguments \
                        '--help[Show help information]' \
                        '--json[Output results in JSON format]' \
                        '--verbose[Enable verbose output]' \
                        '--quiet[Suppress non-essential output]'
                    ;;
                maps:validate)
                    _arguments \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--quiet[Suppress non-essential output]'
                    ;;
                maps:list)
                    _arguments \
                        '--help[Show help information]' \
                        '--json[Output results in JSON format]' \
                        '--verbose[Enable verbose output]'
                    ;;
                maps:check)
                    _arguments \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--quiet[Suppress non-essential output]'
                    ;;
                daw:generate)
                    _arguments \
                        '--target[Specify target DAW]:target:(ardour reaper ableton logic)' \
                        '--install[Install generated files to target location]' \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--output[Specify output directory]:output:_directories' \
                        '--dry-run[Show what would be done without executing]'
                    ;;
                daw:generate:ardour)
                    _arguments \
                        '--install[Install generated files to Ardour]' \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--output[Specify output directory]:output:_directories' \
                        '--dry-run[Show what would be done without executing]'
                    ;;
                daw:list)
                    _arguments \
                        '--help[Show help information]' \
                        '--json[Output results in JSON format]' \
                        '--verbose[Enable verbose output]'
                    ;;
                workflow:complete)
                    _arguments \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--force[Force operation, bypass cache/confirmations]' \
                        '--quiet[Suppress non-essential output]'
                    ;;
                workflow:health)
                    _arguments \
                        '--help[Show help information]' \
                        '--verbose[Enable verbose output]' \
                        '--json[Output results in JSON format]' \
                        '--force[Force full health check]' \
                        '--quiet[Suppress non-essential output]'
                    ;;
                *)
                    _describe 'flags' common_flags
                    ;;
            esac
            ;;
    esac
}

# Register the completion function
_audio_control_completion "$@"