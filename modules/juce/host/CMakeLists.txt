juce_add_console_app(ol_juce_host PRODUCT_NAME "OL Juce Plugin Host")
target_sources(ol_juce_host
        PRIVATE
        host.cpp
)
target_compile_definitions(ol_juce_host
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)

target_link_libraries(ol_juce_host
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        juce_audio_processors
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)