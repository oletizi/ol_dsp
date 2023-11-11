add_library(fxlib ol_fxlib.h
        FxControlPanel.h
        FxControlPanel.cpp
        ol_fxlib_core.h
        FxChain.h
        FxChain.cpp
        Delay.cpp
        Delay.h)
target_link_libraries(fxlib DaisySP)
target_include_directories(fxlib PUBLIC . DaisySP/Source)