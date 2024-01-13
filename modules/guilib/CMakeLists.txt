add_library(ol_guilib INTERFACE ol_guilib.h
        SynthApp.h
        ol_guilib_core.h)
target_link_libraries(ol_guilib INTERFACE spline ol_corelib)
target_include_directories(ol_guilib INTERFACE ..)
set_target_properties(ol_guilib PROPERTIES LINKER_LANGUAGE CXX)
