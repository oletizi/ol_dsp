add_library(ol_fxlib
        ../corelib/cc_map.h
        Fx.h
        Reverb.cpp
        ol_fxlib.h
)
target_link_libraries(ol_fxlib ol_corelib ol_ctllib DaisySP dattorro-verb)
target_include_directories(ol_fxlib PUBLIC .)