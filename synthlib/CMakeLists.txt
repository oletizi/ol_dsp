add_library(synthlib Voice.cpp
        Control.cpp
        ControlPanel.h)
target_link_libraries(synthlib DaisySP)
target_include_directories(synthlib PUBLIC .)