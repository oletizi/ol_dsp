add_library(ol_daisy_ui
        ui/Page.cpp
        ui/ui.h
)
target_include_directories(ol_daisy_ui PUBLIC .)


add_executable(ol_daisy_multifx fx/multifx/main.cpp)
target_link_libraries(ol_daisy_multifx daisy DaisySP ol_daisy_ui ol_corelib ol_fxlib)


