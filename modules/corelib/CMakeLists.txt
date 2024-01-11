add_library(ol_corelib ol_corelib.h
        ol_corelib.cpp)
target_include_directories(ol_corelib PUBLIC .. .)
set_target_properties(ol_corelib PROPERTIES LINKER_LANGUAGE CXX)