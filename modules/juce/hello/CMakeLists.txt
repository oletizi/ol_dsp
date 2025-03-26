add_executable(ol_hello hello.cpp)
juce_add_console_app(ol_juce_hello PRODUCT_NAME "OL Juce Hello")
target_sources(ol_juce_hello
        PRIVATE
        juce_hello.cpp
)
target_compile_definitions(ol_juce_hello
        PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)

target_link_libraries(ol_juce_hello
        PRIVATE
        # ConsoleAppData            # If you'd created a binary data target, you'd link to it here
        juce::juce_core
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)