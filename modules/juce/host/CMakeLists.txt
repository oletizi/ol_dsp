juce_add_console_app(ol_juce_host
        PRODUCT_NAME "OL Juce Plugin Host"
        PLUGINHOST_AU TRUE
        FORMATS Standalone VST3 AU
)
target_sources(ol_juce_host
        PRIVATE
        host.cpp
)
target_compile_definitions(ol_juce_host
        PRIVATE
        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_VST3=1
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)

target_link_libraries(ol_juce_host
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_audio_processors
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)




juce_add_console_app(ai_juce_host
        PRODUCT_NAME "AI Juce Plugin Host"
        PLUGINHOST_AU TRUE
        FORMATS Standalone VST3 AU
)
target_sources(ai_juce_host
        PRIVATE
        PluginHost.cpp
)
target_compile_definitions(ai_juce_host
        PRIVATE
        JUCE_PLUGINHOST_LV2=1
        JUCE_PLUGINHOST_VST3=1
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)

target_link_libraries(ai_juce_host
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        juce::juce_audio_devices
        juce::juce_audio_processors
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)