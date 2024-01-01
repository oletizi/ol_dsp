add_library(ol_ctllib
        ol_ctllib.h
        ol_ctllib_core.h
        ControlThingyDEPRECATE_ME.h
        ol_ctllib.cpp)
target_link_libraries(ol_ctllib ol_corelib DaisySP)
target_include_directories(ol_ctllib PUBLIC . ..)