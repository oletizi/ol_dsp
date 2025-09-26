#!/bin/bash
# audio-control bash completion script
# Generated for audio-control monorepo project

_audio_control_completion() {
    local cur prev words cword
    _init_completion || return

    # pnpm command structure: pnpm [script] [flags]
    # We complete script names and common flags
    
    local scripts=(
        "plugins:extract"
        "plugins:list"
        "plugins:health"
        "maps:validate"
        "maps:list"
        "maps:check"
        "daw:generate"
        "daw:generate:ardour"
        "daw:list"
        "workflow:complete"
        "workflow:health"
    )
    
    local common_flags=(
        "--help"
        "--force"
        "--install"
        "--json"
        "--verbose"
        "--target"
        "--output"
        "--dry-run"
        "--quiet"
    )
    
    local daw_targets=(
        "ardour"
        "reaper"
        "ableton"
        "logic"
    )

    # If we're completing after 'pnpm'
    if [[ $cword -eq 1 && "${words[0]}" == "pnpm" ]]; then
        COMPREPLY=($(compgen -W "${scripts[*]}" -- "$cur"))
        return 0
    fi
    
    # If we're completing flags
    if [[ "$cur" == -* ]]; then
        # Special handling for scripts that support specific flags
        case "${words[1]}" in
            "plugins:extract")
                local specific_flags=("--force" "--help" "--verbose" "--json" "--output" "--dry-run")
                COMPREPLY=($(compgen -W "${specific_flags[*]}" -- "$cur"))
                ;;
            "daw:generate" | "daw:generate:ardour")
                local specific_flags=("--target" "--install" "--help" "--verbose" "--json" "--output" "--dry-run")
                COMPREPLY=($(compgen -W "${specific_flags[*]}" -- "$cur"))
                ;;
            "maps:validate" | "maps:check")
                local specific_flags=("--help" "--verbose" "--json" "--quiet")
                COMPREPLY=($(compgen -W "${specific_flags[*]}" -- "$cur"))
                ;;
            "workflow:complete" | "workflow:health")
                local specific_flags=("--help" "--verbose" "--json" "--force" "--quiet")
                COMPREPLY=($(compgen -W "${specific_flags[*]}" -- "$cur"))
                ;;
            *)
                COMPREPLY=($(compgen -W "${common_flags[*]}" -- "$cur"))
                ;;
        esac
        return 0
    fi
    
    # Handle --target flag value completion
    if [[ "$prev" == "--target" ]]; then
        COMPREPLY=($(compgen -W "${daw_targets[*]}" -- "$cur"))
        return 0
    fi
    
    # Handle file path completion for --output
    if [[ "$prev" == "--output" ]]; then
        _filedir
        return 0
    fi
    
    # Default to script completion if nothing else matches
    COMPREPLY=($(compgen -W "${scripts[*]}" -- "$cur"))
}

# Register completion for pnpm command
complete -F _audio_control_completion pnpm

# Also register for npm in case users have it aliased
complete -F _audio_control_completion npm