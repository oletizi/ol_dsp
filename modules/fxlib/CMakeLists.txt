add_library(ol_fxlib
        cc_map.h
        Fx.h
        Filter.cpp
        Delay.cpp
        Reverb.cpp
        FxRack.cpp
        Saturator.cpp
)
target_link_libraries(ol_fxlib ol_corelib ol_ctllib DaisySP dattorro-verb soundpipe)
target_include_directories(ol_fxlib PUBLIC .)