add_library(ol_fxlib ol_fxlib.h
        FxControlPanel.h
        FxControlPanel.cpp
        ol_fxlib_core.h
        FxChain.h
        FxChain.cpp
        Delay.cpp
        Delay.h
        LPF.cpp
        LPF.h
        Reverb.cpp
        Reverb.h
        ReverbControlPanel.cpp
        ReverbControlPanel.h
        Fx.h)
target_link_libraries(ol_fxlib ol_corelib ol_ctllib DaisySP)
target_include_directories(ol_fxlib PUBLIC .)